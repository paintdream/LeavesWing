# Copilot Instructions

## Project Structure Guidelines
- All code, comments and documents should be written in English.
- Do not modify other directories on the project's root.

## Git Guidelines
- All commit messages must start with [DOC], [ADD], [MOD], [DEL], [FIX] with an extra space.

## Coding Guidelines
- Except for third-party libraries, which retain their original code style: the code style is unified as camelCase for variable names and PascalCase for function and class names; indentation must use TABs.
- When referencing functions/variables from other plugins, you must ensure functions/variables are dll-exported or inlined.
- Use `QT_DIR` rather than `QTDIR` for Qt-related environment variable naming in this repository.
