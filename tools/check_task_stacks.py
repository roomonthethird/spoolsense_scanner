"""Scan ESP32 source files for FreeRTOS task creation and flag undersized stacks.

Adapted from esp32-claude-workbench for PlatformIO/Arduino projects.
Supports .c, .cpp, .h, .hpp files.

Usage:
    python3 tools/check_task_stacks.py src/
    python3 tools/check_task_stacks.py .
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class TaskInfo:
    """Parsed FreeRTOS task creation info."""

    name: str
    stack_size: int | None
    stack_expr: str
    priority: str
    core: str | None
    function: str
    file: str
    line: int


@dataclass
class StackIssue:
    """A detected stack sizing issue."""

    severity: str  # ERROR, WARNING, INFO
    task_name: str
    message: str
    file: str
    line: int


SOURCE_EXTENSIONS = {".c", ".cpp", ".h", ".hpp"}

# Minimum recommended stack sizes by task type
STACK_THRESHOLDS = {
    "minimal": 1024,
    "simple": 2048,
    "networking": 4096,
    "tls_https": 8192,
    "json_heavy": 8192,
}

# Keywords that indicate a task needs more stack
HEAVY_KEYWORDS = {
    "wifi": 4096,
    "http": 8192,
    "https": 8192,
    "tls": 8192,
    "ssl": 8192,
    "mqtt": 4096,
    "ble": 4096,
    "bluetooth": 4096,
    "ota": 8192,
    "json": 4096,
    "spoolman": 8192,  # SpoolSense: HTTP + JSON parsing
    "nfc": 4096,       # SpoolSense: NFC operations
    "scan": 4096,      # SpoolSense: NFC scan task
    "printer": 4096,   # SpoolSense: printer management
    "websocket": 8192,
    "web": 8192,       # SpoolSense: web server
    "display": 4096,
    "lcd": 3072,
    "tft": 4096,
    "gui": 8192,
    "lvgl": 8192,
}

# xTaskCreatePinnedToCore(func, "name", stack, param, priority, handle, core)
TASK_CREATE_PINNED_PATTERN = re.compile(
    r"xTaskCreatePinnedToCore\s*\(\s*"
    r"(\w+)\s*,\s*"           # function
    r'"([^"]*?)"\s*,\s*'      # task name
    r"([^,]+?)\s*,\s*"        # stack size
    r"[^,]+?\s*,\s*"          # parameter
    r"([^,]+?)\s*,\s*"        # priority
    r"[^,]+?\s*,\s*"          # handle
    r"([^)]+?)\)",            # core
    re.DOTALL,
)

# xTaskCreate(function, "name", stack, param, priority, handle)
TASK_CREATE_PATTERN = re.compile(
    r"xTaskCreate\s*\(\s*"
    r"(\w+)\s*,\s*"           # function
    r'"([^"]*?)"\s*,\s*'      # task name
    r"([^,]+?)\s*,\s*"        # stack size
    r"[^,]+?\s*,\s*"          # parameter
    r"([^,]+?)\s*,\s*"        # priority
    r"[^)]+?\)",              # handle
    re.DOTALL,
)


def try_parse_int(expr: str) -> int | None:
    """Try to parse a stack size expression as an integer."""
    expr = expr.strip()

    try:
        return int(expr)
    except ValueError:
        pass

    # Simple multiplication: X * Y
    mult_match = re.match(r"(\d+)\s*\*\s*(\d+)", expr)
    if mult_match:
        return int(mult_match.group(1)) * int(mult_match.group(2))

    # Class constant: TASK_STACK_SIZE, STACK_SIZE, etc.
    # Can't resolve without reading the class — return None
    return None


def scan_file(filepath: Path) -> list[TaskInfo]:
    """Scan a source file for task creation calls."""
    tasks: list[TaskInfo] = []
    try:
        content = filepath.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return tasks

    # Also try to resolve constexpr stack size constants
    constants: dict[str, int] = {}
    for match in re.finditer(
        r"static\s+constexpr\s+(?:size_t|uint32_t|int)\s+(\w*STACK\w*)\s*=\s*(\d+)",
        content,
    ):
        constants[match.group(1)] = int(match.group(2))

    for match in TASK_CREATE_PINNED_PATTERN.finditer(content):
        pos = match.start()
        line_num = content[:pos].count("\n") + 1

        stack_expr = match.group(3).strip()
        stack_size = try_parse_int(stack_expr)
        # Try resolving from constants
        if stack_size is None and stack_expr in constants:
            stack_size = constants[stack_expr]

        tasks.append(
            TaskInfo(
                function=match.group(1).strip(),
                name=match.group(2).strip(),
                stack_size=stack_size,
                stack_expr=stack_expr,
                priority=match.group(4).strip(),
                core=match.group(5).strip(),
                file=str(filepath),
                line=line_num,
            )
        )

    for match in TASK_CREATE_PATTERN.finditer(content):
        pos = match.start()
        line_num = content[:pos].count("\n") + 1

        if "PinnedToCore" in content[pos : pos + 30]:
            continue

        stack_expr = match.group(3).strip()
        stack_size = try_parse_int(stack_expr)
        if stack_size is None and stack_expr in constants:
            stack_size = constants[stack_expr]

        tasks.append(
            TaskInfo(
                function=match.group(1).strip(),
                name=match.group(2).strip(),
                stack_size=stack_size,
                stack_expr=stack_expr,
                priority=match.group(4).strip(),
                core=None,
                file=str(filepath),
                line=line_num,
            )
        )

    return tasks


def analyze_tasks(tasks: list[TaskInfo]) -> list[StackIssue]:
    """Analyze task stack sizes for potential issues."""
    issues: list[StackIssue] = []

    for task in tasks:
        if task.stack_size is None:
            issues.append(
                StackIssue(
                    severity="INFO",
                    task_name=task.name,
                    message=f"Stack size is '{task.stack_expr}' — cannot verify automatically",
                    file=task.file,
                    line=task.line,
                )
            )
            continue

        if task.stack_size < STACK_THRESHOLDS["minimal"]:
            issues.append(
                StackIssue(
                    severity="ERROR",
                    task_name=task.name,
                    message=f"Stack {task.stack_size} bytes is below minimum ({STACK_THRESHOLDS['minimal']})",
                    file=task.file,
                    line=task.line,
                )
            )
            continue

        combined = f"{task.name} {task.function}".lower()
        for keyword, min_stack in HEAVY_KEYWORDS.items():
            if keyword in combined and task.stack_size < min_stack:
                issues.append(
                    StackIssue(
                        severity="WARNING",
                        task_name=task.name,
                        message=(
                            f"Task appears to use {keyword.upper()} "
                            f"(stack: {task.stack_size}) — "
                            f"recommended minimum: {min_stack} bytes"
                        ),
                        file=task.file,
                        line=task.line,
                    )
                )
                break

        if task.stack_size < STACK_THRESHOLDS["simple"]:
            issues.append(
                StackIssue(
                    severity="WARNING",
                    task_name=task.name,
                    message=f"Stack {task.stack_size} bytes is small — minimum recommended: {STACK_THRESHOLDS['simple']}",
                    file=task.file,
                    line=task.line,
                )
            )

    return sorted(
        issues, key=lambda i: 0 if i.severity == "ERROR" else 1 if i.severity == "WARNING" else 2
    )


def format_report(tasks: list[TaskInfo], issues: list[StackIssue]) -> str:
    """Format task stack analysis as markdown."""
    lines: list[str] = []
    lines.append("# Task Stack Analysis Report")
    lines.append("")

    error_count = sum(1 for i in issues if i.severity == "ERROR")
    warn_count = sum(1 for i in issues if i.severity == "WARNING")

    lines.append("## Summary")
    lines.append(f"- **Tasks found**: {len(tasks)}")
    lines.append(f"- **Errors**: {error_count}")
    lines.append(f"- **Warnings**: {warn_count}")
    lines.append("")

    if tasks:
        lines.append("## Task Inventory")
        lines.append("")
        lines.append("| Task Name | Function | Stack | Priority | Core | File |")
        lines.append("|-----------|----------|-------|----------|------|------|")
        for t in tasks:
            stack_str = str(t.stack_size) if t.stack_size else f"`{t.stack_expr}`"
            core_str = str(t.core) if t.core is not None else "any"
            short_file = Path(t.file).name
            lines.append(
                f"| {t.name} | `{t.function}` | {stack_str} | {t.priority} | {core_str} | {short_file}:{t.line} |"
            )
        lines.append("")

    if issues:
        lines.append("## Issues")
        lines.append("")
        for issue in issues:
            icon = {"ERROR": "ERROR", "WARNING": "WARN", "INFO": "INFO"}[issue.severity]
            short_file = Path(issue.file).name
            lines.append(f"- [{icon}] **{issue.task_name}**: {issue.message}")
            lines.append(f"  - `{short_file}:{issue.line}`")
        lines.append("")
    else:
        lines.append("## No Issues Found")
        lines.append("")

    return "\n".join(lines)


def scan_directory(directory: Path) -> tuple[list[TaskInfo], list[StackIssue]]:
    """Scan all source files in a directory for task creations."""
    all_tasks: list[TaskInfo] = []

    for filepath in directory.rglob("*"):
        if filepath.suffix not in SOURCE_EXTENSIONS:
            continue
        if "build" in filepath.parts or ".pio" in filepath.parts or ".git" in filepath.parts:
            continue
        all_tasks.extend(scan_file(filepath))

    issues = analyze_tasks(all_tasks)
    return all_tasks, issues


def main() -> None:
    """CLI entry point."""
    if len(sys.argv) < 2:
        print("Usage: python3 tools/check_task_stacks.py <directory>")
        print("       python3 tools/check_task_stacks.py src/")
        sys.exit(1)

    directory = Path(sys.argv[1])
    if not directory.is_dir():
        print(f"ERROR: Not a directory: {directory}")
        sys.exit(1)

    tasks, issues = scan_directory(directory)
    print(format_report(tasks, issues))

    error_count = sum(1 for i in issues if i.severity == "ERROR")
    if error_count > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
