import tkinter as tk
from tkinter import messagebox
import subprocess
import threading
import os
import signal

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

        self.http_history_listbox = tk.Listbox(self.history_frame, height=10, width=80)
        self.http_history_listbox.grid(row=0, column=0, padx=5, pady=5)

        self.scrollbar = tk.Scrollbar(self.history_frame)
        self.scrollbar.grid(row=0, column=1, sticky='ns')
        self.http_history_listbox.config(yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.http_history_listbox.yview)

        self.log_frame = tk.LabelFrame(self.root, text="Log", padx=10, pady=10)
        self.log_frame.grid(row=2, column=0, padx=10, pady=10, sticky="nsew")

        self.log_text = tk.Text(self.log_frame, height=10, width=80)
        self.log_text.grid(row=0, column=0, padx=5, pady=5)

        self.proxy_process = None

    def toggle_intercept(self):
        if self.proxy_process is None:
            self.start_proxy_server()
        else:
            self.stop_proxy_server()

    def forward_packet(self):
        self.send_response("FORWARD")
        self.log_message("Forward button pressed. Packet forwarded.")

    def drop_packet(self):
        self.send_response("DROP")  # Instruiește proxy-ul să închidă conexiunea
        self.log_message("Drop button pressed. Connection closed.")

    def log_message(self, message):
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.yview(tk.END)

    def start_proxy_server(self):
        self.proxy_process = subprocess.Popen(
            ['/home/andreea/Desktop/HTTP-Proxy/my_program'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        self.log_message("Proxy server started.")
        self.intercept_on_button.config(text="Intercept Off")

        # Porneste un thread de ascultare a cererilor
        self.listener_thread = threading.Thread(target=self.listen_to_requests, daemon=True)
        self.listener_thread.start()

    def stop_proxy_server(self):
        if self.proxy_process:
            self.proxy_process.terminate()  # Oprește procesul
            self.proxy_process.wait()  # Așteaptă ca procesul să se termine complet
            self.proxy_process = None  # Resetează variabila
            self.log_message("Proxy server stopped.")
            self.intercept_on_button.config(text="Intercept On")

    # Thread ce citește continuu din PIPE_REQUEST cererile interceptate și le afișează în HTTP history
    def listen_to_requests(self):
        with open(PIPE_REQUEST, "r") as request_pipe:
            while True:
                request = request_pipe.readline()
                if request:
                    self.http_history_listbox.insert(tk.END, request)
                    self.log_message(f"Intercepted Request:\n{request}")

    def send_response(self, response):
        try:
            self.log_message(f"Sending response to proxy: {response}")  # Log
            with open(PIPE_RESPONSE, "w") as response_pipe:
                response_pipe.write(response + "\n")
            self.log_message(f"Response sent: {response}")
        except Exception as e:
            self.log_message(f"Error sending response: {e}")

    def on_close(self):
        if self.proxy_process:
            self.stop_proxy_server()  # Asigură-te că procesul proxy este oprit
        self.root.destroy()  # Închide fereastra GUI

    def run_gui(self):
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.root.mainloop()

if __name__ == "__main__":
    root = tk.Tk()
    app = ProxyApp(root)
    app.run_gui()  