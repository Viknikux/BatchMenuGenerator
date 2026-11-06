# BatchMenuGenerator

![Version](https://img.shields.io/badge/version-1.5.0-blue)
![Status](https://img.shields.io/badge/status-release--stable-success)
![Platform](https://img.shields.io/badge/platform-Windows-blue)

**BatchMenuGenerator (BMG)** is a lightweight, powerful utility for creating, editing and compiling Windows `.bat` menu scripts. What started as a simple question-and-answer wizard has evolved into a complete command-line workbench featuring interactive arrow-key menus, nested submenus, a built-in text editor, persistent settings and an integrated EXE compiler.

Create everything from simple command launchers to complex, multi-level menu systems with custom commands, administrator privileges and password protection.

> 🐧 **Looking for Linux support?** Check out the native Linux port: [BashMenuGenerator](https://github.com/Viknikux/BashMenuGenerator)!

---

## 🚀 Features

### 🖥️ Interactive Menu System

- **Arrow-Key Interface (TUI):** Navigate menus using Up/Down, Enter and Esc instead of constantly typing numbers or answering Y/N prompts.
- **Nested Submenus:** Create hierarchical menu systems up to 6 levels deep, with up to 20 options per menu.
- **Custom Menu Appearance:** Choose between Classic (`=`), Modern (`-`) and Zigzag (`Z`) borders.
- **Built-in Theme Engine:** Customize the entire interface with different color presets, including Default, Matrix, Cyberpunk, CLI and Violent.
- **Optional Menu Sounds:** Enable subtle navigation and confirmation sounds.
- **Classic Prompt Mode:** Prefer the traditional keyboard input experience? Disable TUI Mode in Settings.

### ⚙️ Powerful Script Generation

- **Multiple Run Modes:** Execute commands directly, through a `cmd /c` wrapper, or open a nested submenu.
- **Command Chaining:** Assign multiple commands to individual menu options. Use commands such as `timeout /t` to introduce execution delays.
- **Administrator Privileges:** Automatically request and relaunch with elevated Administrator privileges when required.
- **Password Protection:** Add password verification to generated batch menus.
- **Custom Titles & Descriptions:** Create personalized menu headers, descriptions and even ASCII-art banners.
- **Automatic Script Formatting:** Special characters are properly escaped to prevent generated scripts from breaking.

### ✏️ Built-in Text Editor

A full-screen console text editor is integrated directly into BatchMenuGenerator.

- Edit text configurations, command lists, descriptions and existing text files.
- Undo and redo with up to 200 history steps.
- Find text using `Ctrl + F` and repeat searches with `F3`.
- Select text using `Shift + Arrow Keys` or `Ctrl + A`.
- Copy, cut and paste using familiar keyboard shortcuts.
- Navigate and delete entire words with Ctrl-based shortcuts.
- Save using `Ctrl + S`.
- Live status bar displaying line, column and selection information.

### 📄 Text Configuration Mode

Build complete menu systems using plain-text configuration files instead of answering wizard questions.

- Edit configurations directly inside the built-in editor or Notepad.
- Load existing `.txt` configurations.
- Supports UTF-8 (with or without BOM) and UTF-16 LE.
- Define menu options, commands, custom borders and nested submenus.
- Built-in syntax tutorial with examples.
- Previous configurations remain compatible with older versions.

### 🔄 Edit Existing Scripts

Already generated a batch menu? You don't have to start over!

Open previously generated `.bat` scripts and modify their contents directly.

- Edit titles, descriptions and passwords.
- Enable or disable administrator privileges.
- Change menu borders.
- Add, remove and reorganize menu options.
- Edit commands and run modes.
- Create and modify nested submenus.
- Compile the modified script directly into an EXE.

**Backward compatibility:** BatchMenuGenerator supports previously generated scripts from older versions of BMG, including legacy releases. Compatibility has been tested with scripts dating back to version 0.1.2.

Scripts must retain the `Created with BatchMenuGenerator!` watermark on the first line to be recognized by the script editor.

### 📦 Built-in EXE Compiler

Convert generated batch menus into standalone `.exe` executables without relying on external converters.

- Compile directly after generating a script.
- Compile from the Batch Script Editor.
- Compile through Text Mode configuration.
- Compile any `.bat` file using the command line.
- Automatically uses the .NET Framework compiler (`csc.exe`) when available, with a Windows PowerShell 5.1 fallback.
- Passes command-line arguments to the original batch script.
- Returns the original script's exit code.
- Automatically removes temporary extracted files after execution.

> ⚠️ **Security note:** The embedded batch script is scrambled using a randomly generated key for each build. This is basic obfuscation, NOT encryption. It should not be considered a secure way to protect passwords, credentials or other sensitive information.

### 💾 Settings & AutoSave

- **Persistent Settings:** Save your preferred configuration between launches.
- **Smart AutoSave:** Automatically preserve unfinished sessions and resume where you left off.
- **End Action:** Choose what happens after saving a script: ask, return to the main menu, launch the script or exit.
- **Save/Load Preferences:** Automatically use the Windows File Explorer dialog or manual path input.
- **Default Menu Borders:** Skip repeated border selection.
- **Theme Preferences:** Keep your preferred interface colors.
- **Version History:** View the complete built-in changelog directly inside the application.

Settings are stored in:

```text
%LOCALAPPDATA%\BatchMenuGenerator\settings.txt
```

The configuration uses simple `key: value` lines and can be edited manually.

### 🛠️ Program Setup

BatchMenuGenerator includes an optional first-time setup utility.

- Move the program into `%LOCALAPPDATA%\BatchMenuGenerator`.
- Create a desktop shortcut.
- Add the program to the Windows Start Menu.
- Install, update or uninstall the `bmg` command.
- Automatically configure your user PATH without requiring administrator privileges.

### 💻 Command-Line Support

Control BatchMenuGenerator directly from the terminal using its built-in command-line switches.

| Command | Description |
|---|---|
| `bmg -cli` | Open the built-in text editor immediately. |
| `bmg -config "file.txt"` | Load a configuration and generate its menu. |
| `bmg -edit "file"` | Edit a generated batch script or ordinary text file. |
| `bmg -compile "script.bat"` | Compile a batch file into an EXE without opening the menus. |

You can use `/` instead of `-` for switches.

The compiler returns exit codes that can be used in other scripts:

- `0` – Compilation successful.
- `1` – Compilation failed.
- `2` – Invalid usage or file not found.

Once the `bmg` command is installed, you can launch the application from any terminal location.

---

## 🪟 Compatibility & System Requirements

<sub>No meaningful hardware requirements!</sub>

BatchMenuGenerator is designed to run on practically any Windows computer, including very old and low-specification systems.

### Hardware Requirements

There are effectively no specific hardware requirements. If your computer can run Windows and open a console window, it should be capable of running BatchMenuGenerator.

### Operating System Compatibility

- **Minimum supported OS: Windows Vista (x86).**
- Supports Windows Vista, 7, 8, 8.1, 10 and 11.
- Compatible with both 32-bit (x86) and 64-bit (x64) Windows, provided you download the correct executable.
- No administrator privileges required for normal application usage.
- No separately installed runtime dependencies required for normal operation.

### Important: Download the Correct Architecture!

| Your Windows installation | Download |
|---|---|
| 32-bit Windows (x86) | **x86 executable** |
| 64-bit Windows (x64) | **x64 executable** |

> ⚠️ A 32-bit Windows installation cannot run the x64 executable. If you're using Windows Vista x86 or another 32-bit Windows installation, make sure to download the **x86 version**.

---

## 📸 Preview

### Standard CLI Version (Recommended ✅)

The primary and actively developed version of BatchMenuGenerator.

<img width="537" alt="Standard CLI Interface" src="https://github.com/user-attachments/assets/28676661-0101-482b-ae35-d3b4460efc8c" />

### GUI Version (⚠️ Experimental / Heavy Beta)

> **Note:** The GUI version is currently experimental and unstable. Active development is focused on improving and maintaining the standard CLI version.

<img width="859" alt="GUI Interface Beta" src="https://github.com/user-attachments/assets/fc820163-dff1-4f5b-8ded-4e0cf519618d" />

---

## 🔄 What's New in 1.5.0?

Version 1.5.0 is a major upgrade over 1.4.3.

The application has evolved from a simple question-and-answer wizard into a complete script-generation workbench.

### Major Additions

1. Nested submenu support (up to 6 levels).
2. Interactive arrow-key navigation throughout the application.
3. Persistent settings and customizable TUI themes.
4. Built-in batch-to-EXE compiler.
5. Ability to reopen and edit previously generated batch scripts.
6. Full-screen built-in console text editor.
7. Improved Text Mode with expanded configuration syntax.
8. New command-line switches for automation.
9. First-time setup utility and automatic `bmg` command installation.
10. Automatic compatibility handling for Windows Terminal and the classic Windows console.

### Additional Improvements

- Better escaping of special characters in generated scripts.
- Improved handling of long menu labels.
- Case-insensitive menu choices.
- Improved administrator privilege relaunch behavior.
- Better AutoSave and session recovery.
- Improved save-conflict handling.
- Live title preview.
- Preserved indentation in descriptions and ASCII-art banners.
- Improved configuration loading and compatibility.

### Backward Compatibility

Existing projects are not left behind!

BatchMenuGenerator 1.5.0 maintains backward compatibility with scripts and text configurations created using previous versions of BatchMenuGenerator.

- Previously generated scripts from older BMG releases remain supported.
- Legacy scripts, including those created with version 0.1.2, have been tested successfully.
- Older scripts can be reopened and edited using the Batch Script Editor.
- Existing Run Modes 0 and 1 retain their previous behavior.
- Existing `cmd /c` options remain supported.

---

## 🤝 Contributing & Bug Reports

Found a bug, encountered an issue or have an idea for a new feature?

Feel free to submit a report or suggestion through the repository's **Issues** tab.

Pull requests, improvements and feature suggestions are always welcome!

If you find BatchMenuGenerator useful, please consider giving the repository a **Star ⭐**. It helps support development and makes the project more visible to other users.

---

<p align="center">
  <b>BatchMenuGenerator</b><br>
  Making Windows batch automation easier, one menu at a time.
</p>
