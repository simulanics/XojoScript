# XojoDevKit Quick Start Guide

**Download in the Release Section**

The **XojoDevKit** is a portable C++ compiler toolset for Windows, pre-configured to compile C++ programs and libraries for open source XojoScript. This build system is a modified version of [w64devkit](https://github.com/skeeto/w64devkit). Although not required to build libraries for OS XojoScript, it provides a "3 Minute" setup to start compiling XojoScript, its plugins, and to develop your own projects.

Any language capable of exporting C links is cross-compatible with OS XojoScript. This flexibility allows you to create plugins using Go, Rust, C#, C++, C, or any language you prefer. OS XojoScript benefits from an extensive ecosystem of libraries, functions, tools, and accessories, facilitating rapid integration of cross-platform plugins, as well as OS-specific plugins.

An extensive set of include files, libraries, and advanced features have been integrated into this build. Setting up this environment requires just 10 simple steps and approximately 3–5 minutes, compared to an estimated 5–7 hours for a manual setup on a Microsoft Windows-based machine.

---

## Setup Instructions for Windows

1. **Extract Files**  
   Download and extract the `xojodevkit` folder from the archive. Place it on the C:\ drive (or any desired location). If a location other than C:\ is chosen, update the xojoscript and plugin build scripts to reference the correct path.

2. **Open System Environment Variables**  
   Click the Windows icon on your taskbar, type "environment", and select **Edit the system environment variables**.

3. **Access Environment Variables**  
   In the System Properties window, click the **Environment Variables...** button.

4. **Edit the Path Variable**  
   In the Environment Variables window, locate the **Path** variable in the first list, select it, and click **Edit**.

5. **Add XojoDevKit Bin Directory**  
   In the "Edit environment variable" window, click **New** and add the following path:
   ```
   C:\xojodevkit\bin
   ```

6. **Save Changes**  
   Click **OK** to close the "Edit environment variable" window.

7. **Apply Environment Variables**  
   Click **OK** in the Environment Variables window to apply the changes.

8. **Close System Properties**  
   Click **OK** in the System Properties window to complete the setup.

9. **Verify Setup**  
   Confirm that your system environment is updated to include the XojoDevKit bin directory.

10. **Start Compiling**  
    You are now ready to compile XojoScript, its plugins, or any other C++ programs using the open source g++ compiler on Windows.
