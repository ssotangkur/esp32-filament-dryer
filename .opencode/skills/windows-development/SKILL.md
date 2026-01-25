---
name: windows-development
description: Windows 11 development environment and PowerShell command usage
---

## Windows Environment Rules

### Operating System: Windows 11

This project is developed on Windows 11 with Visual Studio Code as the primary IDE.

## Command Execution

All CLI commands must be executed using PowerShell syntax and commands.

### Correct Usage:
- Use PowerShell cmdlets and syntax for all command-line operations
- Avoid bash or Unix-style commands unless explicitly compatible
- When executing commands, ensure they are formatted for PowerShell

### Why this is necessary:
1. The default shell environment is cmd.exe, but PowerShell provides better scripting capabilities
2. PowerShell offers consistent command syntax across different Windows systems
3. It supports more advanced automation and error handling features
4. Ensures compatibility with Windows-specific tools and environments

### Common PowerShell Commands:
- Get-ChildItem (equivalent to ls/dir)
- Set-Location (equivalent to cd)
- New-Item (equivalent to touch/mkdir)
- Remove-Item (equivalent to rm/del)

### Running Scripts from Any Directory
When creating batch files that need to be run from any directory (like project root), include this line at the beginning of your .bat file to change to the script's directory before executing commands:
```batch
cd /d "%~dp0"
```
This ensures that relative paths in the script work correctly regardless of the current working directory when the script is called. For example, a script in `ui_simulator/build_sim.bat` can be run from the project root with `.\ui_simulator\build_sim.bat` and will automatically change to the `ui_simulator` directory to execute properly.

### Error Prevention:
Running bash-style commands (e.g., `ls`, `cd /path`) directly may fail with errors like:
```
'ls' is not recognized as an internal or external command
```

Always use PowerShell equivalents and syntax for command execution.