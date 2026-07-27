# BatchMenuGenerator

**BatchMenuGenerator (BMG)** is a lightweight, high-performance utility for building Windows `.bat` menu scripts with chained commands, custom launchers, and system automation routines.

> 🐧 **Looking for Linux support?** Check out the native Linux port: [BashMenuGenerator](https://github.com/Viknikux/BashMenuGenerator)!

---

## 🚀 Features

* **Command Mode & Text Configuration:** Input raw commands with an intuitive, easy-to-learn custom syntax. Check out the example `.txt` templates in the main branch to get started!
* **Command Chaining:** Assign multiple commands to a single menu option. Combine commands seamlessly using `timeout /t` for custom execution delays.
* **Admin Privileges Enforcement:** Set your generated scripts to enforce administrative access—scripts automatically request and relaunch with elevated Administrator privileges if missing.
* **Built-in Theme Engine:** Customize your CLI workspace appearance with custom interface color themes.
* **Settings Engine (`bconfig`):** Persistent configuration system to manage global application preferences and default behaviors.
* **Smart AutoSave System:** Automatic session recovery so you never lose your progress during a system crash or accidental exit.
* **Built-in Version History:** View full application changelogs and program version history directly inside the tool.
* **Smart File Handling:** Features automatic illegal character checking, intelligent folder creation, overwrite prompts, and support for both manual file path entry and native Windows File Explorer save dialogs.
* **Optional Password Protection:** Add password verification to your generated batch scripts. *(Tip: To prevent users from viewing password logic via text editors, compile your final script with a tool like Batch-To-EXE-Converter).*
* **Flexible Distribution:** Choose between a portable **Standalone `.exe`** (smaller than a photo with zero installation) or the **Setup Installer** version.

---

## 🪟 Compatibility

Runs out of the box on virtually any Windows environment with **zero extra dependencies**:
* Supported on everything from stripped-down **Windows 7** installs to the latest **Windows 11 (25H2)** builds.
* Portable standalone executable requires no administration rights or pre-installed runtimes.
* The EXE binary is x86 compatible, so minimum OS requirement is Windows 7 x86
---

## 📸 Preview

### Standard CLI Version (Recommended ✅)
<img width="537" alt="Standard CLI Interface" src="https://github.com/user-attachments/assets/28676661-0101-482b-ae35-d3b4460efc8c" />

### GUI Version (⚠️ Experimental / Heavy Beta)
> **Note:** The GUI version is currently unstable. Development is actively focused on optimizing the standard CLI version.

<img width="859" alt="GUI Interface Beta" src="https://github.com/user-attachments/assets/fc820163-dff1-4f5b-8ded-4e0cf519618d" />

---

## 🤝 Contributing & Bug Reports

If you discover any bugs or have ideas for new features, please submit an entry in the **Issues** tab! Pull requests and feature suggestions are always welcome.

If you find BatchMenuGenerator helpful, please **Star ⭐️ this repository** to help support development and reach more users!
