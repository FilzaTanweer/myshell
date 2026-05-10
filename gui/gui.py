#!/usr/bin/env python3
"""
gui/gui.py — Tkinter GUI Frontend for myshell
FAST-NUCES OS Project | Lightweight Frontend Layer

This GUI:
- Launches the compiled myshell executable as a subprocess
- Sends commands via stdin
- Captures output in real-time via stdout/stderr
- Displays in terminal-like text widget
"""

import tkinter as tk
from tkinter import scrolledtext, messagebox
import subprocess
import threading
import os
import sys
from pathlib import Path


class MyShellGUI:
    """Tkinter GUI wrapper for myshell shell process"""

    def __init__(self, root):
        self.root = root
        self.root.title("myshell — GUI Frontend")
        self.root.geometry("800x600")
        self.root.resizable(True, True)

        # Find myshell executable
        self.shell_path = self._find_shell_executable()
        if not self.shell_path:
            messagebox.showerror(
                "Error",
                "myshell executable not found.\n"
                "Please run 'make' in the project root first."
            )
            sys.exit(1)

        self.process = None
        self.output_lock = threading.Lock()
        self.running = False
        self.reader_thread = None

        self._build_ui()
        self._start_shell()

    def _find_shell_executable(self):
        """Locate myshell executable relative to gui.py"""
        # Try current directory, then parent directory
        candidates = [
            Path("./myshell"),
            Path("../myshell"),
            Path(__file__).parent.parent / "myshell",
        ]
        for path in candidates:
            if path.exists() and os.access(path, os.X_OK):
                return str(path.resolve())
        return None

    def _build_ui(self):
        """Construct the GUI components"""
        # Title bar
        title_frame = tk.Frame(self.root, bg="#2c3e50", height=40)
        title_frame.pack(fill=tk.X)

        title_label = tk.Label(
            title_frame,
            text="myshell — Custom Unix Shell (GUI Frontend)",
            bg="#2c3e50",
            fg="white",
            font=("Courier", 10, "bold"),
            pady=10
        )
        title_label.pack()

        # Output display area
        output_label = tk.Label(
            self.root,
            text="Shell Output:",
            font=("Courier", 9),
            anchor="w"
        )
        output_label.pack(fill=tk.X, padx=5, pady=(5, 0))

        self.output_text = scrolledtext.ScrolledText(
            self.root,
            height=20,
            width=100,
            bg="#1e1e1e",
            fg="#00ff00",
            font=("Courier", 10),
            wrap=tk.WORD,
            state=tk.DISABLED
        )
        self.output_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Input frame
        input_frame = tk.Frame(self.root)
        input_frame.pack(fill=tk.X, padx=5, pady=5)

        input_label = tk.Label(
            input_frame,
            text="Command:",
            font=("Courier", 9)
        )
        input_label.pack(side=tk.LEFT, padx=(0, 5))

        self.input_entry = tk.Entry(
            input_frame,
            font=("Courier", 10),
            bg="#f0f0f0"
        )
        self.input_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.input_entry.bind("<Return>", lambda e: self._run_command())

        # Button frame
        button_frame = tk.Frame(self.root)
        button_frame.pack(fill=tk.X, padx=5, pady=5)

        run_button = tk.Button(
            button_frame,
            text="Run",
            command=self._run_command,
            bg="#27ae60",
            fg="white",
            font=("Courier", 10, "bold"),
            width=10
        )
        run_button.pack(side=tk.LEFT, padx=2)

        clear_button = tk.Button(
            button_frame,
            text="Clear",
            command=self._clear_output,
            bg="#e67e22",
            fg="white",
            font=("Courier", 10, "bold"),
            width=10
        )
        clear_button.pack(side=tk.LEFT, padx=2)

        exit_button = tk.Button(
            button_frame,
            text="Exit",
            command=self._exit_shell,
            bg="#e74c3c",
            fg="white",
            font=("Courier", 10, "bold"),
            width=10
        )
        exit_button.pack(side=tk.LEFT, padx=2)

        status_label = tk.Label(
            button_frame,
            text=f"Shell: {self.shell_path}",
            font=("Courier", 8),
            fg="gray"
        )
        status_label.pack(side=tk.RIGHT, padx=5)

    def _start_shell(self):
        """Spawn the myshell subprocess"""
        try:
            self.process = subprocess.Popen(
                [self.shell_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=1,
                universal_newlines=True,
                text=True
            )
            self.running = True

            # Start reader thread to capture output
            self.reader_thread = threading.Thread(
                target=self._read_output,
                daemon=True
            )
            self.reader_thread.start()

            self._append_output("✔ myshell started successfully\n")

        except Exception as e:
            messagebox.showerror("Error", f"Failed to start myshell:\n{e}")
            sys.exit(1)

    def _read_output(self):
        """Read subprocess output in a separate thread (non-blocking)"""
        try:
            for line in iter(self.process.stdout.readline, ""):
                if line:
                    with self.output_lock:
                        self._append_output(line)
        except Exception as e:
            with self.output_lock:
                self._append_output(f"\n[Error reading output: {e}]\n")
        finally:
            self.running = False

    def _append_output(self, text):
        """Thread-safe append to output text widget"""
        self.output_text.config(state=tk.NORMAL)
        self.output_text.insert(tk.END, text)
        self.output_text.see(tk.END)  # Scroll to bottom
        self.output_text.config(state=tk.DISABLED)
        self.root.update_idletasks()

    def _run_command(self):
        """Send command from input entry to shell subprocess"""
        command = self.input_entry.get().strip()
        if not command:
            return

        if not self.running or self.process is None:
            messagebox.showerror("Error", "Shell process not running")
            return

        try:
            self._append_output(f"$ {command}\n")
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()
            self.input_entry.delete(0, tk.END)

        except BrokenPipeError:
            self._append_output("\n[Shell process terminated]\n")
            self.running = False
        except Exception as e:
            self._append_output(f"\n[Error: {e}]\n")

    def _clear_output(self):
        """Clear the output display"""
        self.output_text.config(state=tk.NORMAL)
        self.output_text.delete(1.0, tk.END)
        self.output_text.config(state=tk.DISABLED)

    def _exit_shell(self):
        """Gracefully exit shell and close GUI"""
        if self.running and self.process:
            try:
                self.process.stdin.write("exit\n")
                self.process.stdin.flush()
                self.process.wait(timeout=2)
            except Exception:
                self.process.terminate()

        self.running = False
        self.root.quit()


def main():
    """Entry point"""
    root = tk.Tk()
    app = MyShellGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
EOF
