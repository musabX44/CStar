# C* : Minimal, Extensible Scripting Language Inspired by C and HolyC

Version - 0.0.1

C* is a minimal, extensible scripting language inspired by the syntax and philosophy of C and HolyC. Designed for learning, rapid prototyping, and language tooling, C* aims to provide a simple, approachable, and modifiable language core suitable for education, systems experimentation, and personal projects.  
**This project is closed source. All rights reserved.**

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Philosophy](#philosophy)
- [Language Syntax](#language-syntax)
- [Usage](#usage)
- [Input/Output](#inputoutput)
- [Examples](#examples)
- [Roadmap](#roadmap)
- [License](#license)
- [Contact](#contact)
- [Acknowledgements](#acknowledgements)
- [FAQ](#faq)

---

## Overview

C* is a lightweight scripting language reminiscent of classic C, but with a focus on minimalism, readability, and ease of extension. Inspired by [HolyC](https://en.wikipedia.org/wiki/HolyC), it is implemented in a single file and designed to be both a playground for language designers and a practical tool for scripting and automation.

- Minimal syntax: Easy to learn for beginners, comfortable for C veterans.
- Designed with extensibility in mind: Intended as a foundation for adding new features, experimenting with language design, or building larger tools.
- Educational: Great for compiler and interpreter enthusiasts, or as a teaching aid.

> **Note:**  
> C* is currently a closed-source project. If you are interested in licensing, partnerships, or collaboration, please see [Contact](#contact).

---

## Features

- **C-like Syntax:** Familiar variable declarations, control flow, and function definitions.
- **Simple Variable Declaration:** e.g., `int a = 5;`
- **Basic Data Types:** `int`, `float`, `string`, and more (planned).
- **Input/Output:**  
  - Output: `out.display("Hello, world");`
  - Input: `user.in();`
- **Basic Control Flow:** `if`, `else`, `while`, `for`, and `return` statements.
- **Single File Implementation:** Easy to review, modify, or embed.
- **Extensibility:** Core code is written to be simple to fork and extend.
- **Error Reporting:** Basic error messages for syntax and runtime issues.
- **Closed Source:** All rights reserved. Not open source.

---

## Philosophy

C* is created with the following principles:

- **Minimalism:** Provide only essential language constructs; no bloat.
- **Transparency:** Keep code and language features easy to understand.
- **Extensibility:** Make it easy for users to fork and add new features (upon permission).
- **Learning:** Serve as a learning resource for language and interpreter design.
- **Practicality:** Be useful for small scripting tasks, rapid prototyping, or as a language tools playground.

---

## Language Syntax

### Variable Declarations

```c
int a = 5;
float b = 2.3;
```

### Output

```c
out.display("Hello, world!");
```

### Input

```c
string name = user.in();
```

### Control Flow

```c
if (a > 0) {
    out.display("Positive");
} else {
    out.display("Non-positive");
}

while (a > 0) {
    a = a - 1;
}
```

### Functions

```c
int add(int x, int y) {
    return x + y;
}
```

---

## Usage

> **Note:**  
> As C* is a closed source project, binaries or source code are not publicly available. If you are interested in accessing the language for evaluation, licensing, or educational use, please contact the author.

### Planned Usage (subject to licensing):

- Download or clone the repository (with permission).
- Compile or run the provided interpreter.
- Write your C* scripts and execute them.

---

## Input/Output

- **Output:**  
  Use `out.display()` for printing to the console.
  ```c
  out.display("Result: ", result);
  ```
- **Input:**  
  Use `user.in()` to read input from the user.
  ```c
  string name = user.in();
  ```

---

## Examples

### Hello World

```c
out.display("Hello, world!");
```

### Simple Calculator

```c
int a = user.in();
int b = user.in();
int sum = a + b;
out.display("Sum is: ", sum);
```

### Factorial Function

```c
int fact(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}
out.display(fact(5));
```

---

## Roadmap

- [x] Core interpreter with C-like syntax
- [ ] Expanded data types (`array`, `map`, etc.)
- [ ] File I/O support
- [ ] Module system
- [ ] Error handling improvements
- [ ] Standard library functions (math, string, etc.)
- [ ] Debugger and REPL

---

## License

This project is **closed source**.  
All rights reserved.  
See [LICENSE](./LICENSE) for details.

---

## Contact

For licensing, collaboration, or partnership inquiries:

- **Author:** musabX44
---

## Acknowledgements

- [C programming language](https://en.wikipedia.org/wiki/C_(programming_language))
- [HolyC](https://en.wikipedia.org/wiki/HolyC)
- Open-source language communities for inspiration

---
