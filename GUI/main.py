import tkinter as tk
from tkinter import messagebox
import subprocess
import threading
import os
import signal
import select

PIPE_REQUEST = "/tmp/proxy_request"
PIPE_RESPONSE = "/tmp/proxy_response"

if not os.path.exists(PIPE_REQUEST):
    os.mkfifo(PIPE_REQUEST)

if not os.path.exists(PIPE_RESPONSE):
    os.mkfifo(PIPE_RESPONSE)

class ProxyApp:
    def __init__(self, root):
        self.root = root
        self.root.title("HTTP Proxy GUI")
        self.root.geometry("800x600")

        self.intercept_frame = tk.LabelFrame(self.root, text="Intercept", padx=10, pady=10)
        self.intercept_frame.grid(row=0, column=0, padx=10, pady=10)

        self.intercept_on_button = tk.Button(self.intercept_frame, text="Intercept On", command=self.toggle_intercept)
        self.intercept_on_button.grid(row=0, column=0, padx=5, pady=5)

        self.forward_button = tk.Button(self.intercept_frame, text="Forward", command=self.forward_packet)
        self.forward_button.grid(row=0, column=1, padx=5, pady=5)

        self.drop_button = tk.Button(self.intercept_frame, text="Drop", command=self.drop_packet)
        self.drop_button.grid(row=0, column=2, padx=5, pady=5)

        self.history_frame = tk.LabelFrame(self.root, text="HTTP History", padx=10, pady=10)
        self.history_frame.grid(row=1, column=0, padx=10, pady=10, sticky="nsew")

        self.http_history_listbox = tk.Listbox(self.history_frame, height=15, width=100)
        self.http_history_listbox.grid(row=0, column=0, padx=5, pady=5)

        self.scrollbar = tk.Scrollbar(self.history_frame)
        self.scrollbar.grid(row=0, column=1, sticky='ns')
        self.http_history_listbox.config(yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.http_history_listbox.yview)

        self.log_frame = tk.LabelFrame(self.root, text="Log", padx=10, pady=10)
        self.log_frame.grid(row=2, column=0, padx=10, pady=10, sticky="nsew")

        self.log_text = tk.Text(self.log_frame, height=10, width=100)
        self.log_text.grid(row=0, column=0, padx=5, pady=5)

        self.edit_frame = tk.LabelFrame(self.root, text="Edit Message", padx=10, pady=10)
        self.edit_frame.grid(row=3, column=0, padx=10, pady=10, sticky="nsew")

        self.edit_text = tk.Text(self.edit_frame, height=5, width=100)
        self.edit_text.grid(row=0, column=0, padx=5, pady=5)

        self.proxy_process = None

        # Bind pentru selectarea unui mesaj din history
        self.http_history_listbox.bind("<<ListboxSelect>>", self.load_selected_message)

    def toggle_intercept(self):
        if self.proxy_process is None:
            self.start_proxy_server()
        else:
            self.stop_proxy_server()

    def forward_packet(self):
        # Obține mesajul editat din câmpul de editare
        edited_message = self.edit_text.get(1.0, tk.END).strip()
        if edited_message:
            self.send_response(edited_message)  # Trimite mesajul editat
            self.log_message(f"Forwarded edited message:\n{edited_message}")
        else:
            self.log_message("No message to forward.")

    def drop_packet(self):
        self.send_response("DROP") 
        self.log_message("Drop button pressed. Connection closed.")

    def log_message(self, message):
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.yview(tk.END)

    def history_message(self, message):
        lines = message.splitlines()  # Împarte mesajul în linii
        for line in lines:
            self.http_history_listbox.insert(tk.END, line)  # Adaugă fiecare linie în Listbox
        self.http_history_listbox.yview(tk.END)  # Derulează la final

    def load_selected_message(self, event):
        try:
            selected_index = self.http_history_listbox.curselection()
            if selected_index:
                message = self.http_history_listbox.get(selected_index)
                self.edit_text.delete(1.0, tk.END)  # Șterge textul existent
                self.edit_text.insert(tk.END, message)  # Inserează mesajul selectat
        except Exception as e:
            self.log_message(f"Error loading selected message: {e}")

    def start_proxy_server(self):
        self.proxy_process = subprocess.Popen(
            ['./my_program'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        self.log_message("Proxy server started.")
        self.intercept_on_button.config(text="Intercept Off")

        # Porneste un thread de ascultare a cererilor
        # Ne asiguram ca doar o instanta a unui thread verifica aceasta conditie
        if not hasattr(self, "listener_thread") or not self.listener_thread.is_alive():
            self.listener_thread = threading.Thread(target=self.listen_to_requests, daemon=True)
            self.listener_thread.start()

    def stop_proxy_server(self):
        if self.proxy_process:
            self.proxy_process.terminate()  # Opreste procesul
            self.proxy_process.wait()  # Așteapta ca procesul sa se termine complet
            self.proxy_process = None  # Reseteaza variabila
            self.log_message("Proxy server stopped.")
            self.intercept_on_button.config(text="Intercept On")

    def listen_to_requests(self):
        with open(PIPE_REQUEST, "r") as request_pipe:
            while True:
                ready, _, _ = select.select([request_pipe], [], [], 1)  # Timeout of 1 second
                if ready:
                    request = request_pipe.readline()
                    if request:
                        self.http_history_listbox.insert(tk.END, request)
                        self.log_message(f"Intercepted Request:\n{request}")
                    else: continue 

    def send_response(self, response):
        try:
            self.log_message(f"Sending response to proxy: {response}")  # Log
            # buffering <=> O_NONBLOCK flag
            with open(PIPE_RESPONSE, "w", buffering=1) as response_pipe:
                response_pipe.write(response + "\n")
                response_pipe.flush()
            self.log_message(f"Response sent: {response}")
        except Exception as e:
            self.log_message(f"Error sending response: {e}")

    def on_close(self):
        if self.proxy_process:
            self.stop_proxy_server()  
        self.root.destroy()  

    def run_gui(self):
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.root.mainloop()

if __name__ == "__main__":
    root = tk.Tk()
    app = ProxyApp(root)
    app.run_gui()
