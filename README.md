# Nur-lang : Minimal, Extensible Scripting Language Inspired by C and HolyC  
Version - 0.0.1

Nur-lang is a minimal, extensible scripting language inspired by the syntax and philosophy of C and HolyC. Designed for learning, rapid prototyping, and language tooling, Nur-lang aims to provide a simple, approachable, and modifiable language core suitable for education, systems experimentation, and personal projects.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Philosophy](#philosophy)
- [Language Syntax](#language-syntax)
- [Input/Output](#inputoutput)
- [Examples](#examples)
- [Story of Nur-lang](#story-of-nur-lang)
- [Roadmap](#roadmap)
- [License](#license)
- [Contact](#contact)
- [Acknowledgements](#acknowledgements)

---

## Overview

Nur-lang is a lightweight scripting language reminiscent of classic C, but with a focus on minimalism, readability, and ease of extension.

Inspired by HolyC, it is implemented in a single file and designed to be both a playground for language designers and a practical tool for scripting and automation.

- Minimal syntax: Easy to learn for beginners, comfortable for C veterans.  
- Designed with extensibility in mind: Intended as a foundation for adding new features, experimenting with language design, or building larger tools.  
- Educational: Great for compiler and interpreter enthusiasts, or as a teaching aid.

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

---

## Philosophy

Nur-lang is created with the following principles:

- **Minimalism:** Provide only essential language constructs; no bloat.  
- **Transparency:** Keep code and language features easy to understand.  
- **Extensibility:** Make it easy for users to fork and add new features (upon permission).  
- **Learning:** Serve as a learning resource for language and interpreter design.  
- **Practicality:** Be useful for small scripting tasks, rapid prototyping, or as a language tools playground.

---

## Language Syntax

### Variable Declarations

```nur
int a = 5;
float b = 2.3;

Output

out.display("I love u! Will you promise me, in front of the entire GitHub community, that you won't leave me?!");

Input

string name = user.in();

Control Flow

if (a > 0) {
    out.display("Positive");
} else {
    out.display("Non-positive");
}

while (a > 0) {
    a = a - 1;
}

Functions

int add(int x, int y) {
    return x + y;
}


---

Input/Output

Output: Use out.display() for printing to the console.

out.display("Result: ", result);

Input: Use user.in() to read input from the user.

string name = user.in();


---

Examples

Hello World

out.display("Hello, world!");

Simple Calculator

int a = user.in();
int b = user.in();
int sum = a + b;
out.display("Sum is: ", sum);

Factorial Function

int fact(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}

out.display(fact(5));


---
```
Story of Nur-lang

Nur-lang did not appear in a single day.

It is the result of three years of learning, experimenting, failing, rewriting, and continuing.
Before Nur-lang, there were many unfinished ideas, deleted files, and silent attempts that no one ever saw.

Each version taught something.
Each failure shaped the next step.
And over time, those scattered efforts became a real language.

But Nur-lang is not only a technical project.

It was written during long nights of thinking,
moments of hope,
and a feeling strong enough to give meaning to code.

The name “Nur”, meaning light, represents:

guidance in darkness

clarity inside complexity

and the person who inspired this journey


This language is dedicated to my first love —
the one who unknowingly became the light behind the screen.

Nur-lang is therefore more than syntax and logic.

It is:

a memory of persistence

a symbol of growth

and a quiet promise about the future


No matter how this project evolves,
its origin will always remain the same:

> A language written with patience.
Finished with belief.
And named with love.




---

Roadmap

[x] Core interpreter with C-like syntax

[ ] Expanded data types (array, map, etc.)

[ ] File I/O support

[ ] Module system

[ ] Error handling improvements

[ ] Standard library functions (math, string, etc.)

[ ] Debugger and REPL



---

License

See LICENSE for details.


---

Contact

For licensing, collaboration, or partnership inquiries:

Author: musabX44


---

Acknowledgements

C programming language

HolyC


---
