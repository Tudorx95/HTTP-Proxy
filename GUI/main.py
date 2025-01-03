import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import threading
import os
import signal
import select

PIPE_REQUEST = "/tmp/proxy_request"
PIPE_RESPONSE = "/tmp/proxy_response"

HISTORY_PATH = "./log.txt"


if not os.path.exists(PIPE_REQUEST):
    os.mkfifo(PIPE_REQUEST)

if not os.path.exists(PIPE_RESPONSE):
    os.mkfifo(PIPE_RESPONSE)

class ProxyApp:
    def __init__(self, root):
        self.root = root
        self.root.title("HTTP Proxy GUI")
        self.root.geometry("1920x1080")

         # Create notebook for tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(expand=True, fill='both')

        # Create tabs
        self.proxy_tab = ttk.Frame(self.notebook)
        #self.blacklist_tab = ttk.Frame(self.notebook)
        self.history_tab = ttk.Frame(self.notebook)

        self.notebook.add(self.proxy_tab, text="Proxy")
        #self.notebook.add(self.blacklist_tab, text="Blacklist")
        self.notebook.add(self.history_tab, text="History")

        # Setup each tab
        self.setup_proxy_tab()
        #self.setup_blacklist_tab()
        self.setup_history_tab()

        # Bind tab change event
        self.notebook.bind("<<NotebookTabChanged>>", self.on_tab_changed)

        # Intercept Button State
        self.intercept_enabled = False
        # Server must start at the beggining
        self.start_proxy_server()
        

    def on_tab_changed(self, event):
        """Handle tab change events."""
        selected_tab = event.widget.tab(event.widget.index("current"))["text"]
        if selected_tab == "History":
            self.load_history()

    def setup_proxy_tab(self):
        """Setup the Proxy tab layout and functionality."""
        # Buttons Frame
        self.proxy_button_frame = tk.Frame(self.proxy_tab, padx=10, pady=10)
        self.proxy_button_frame.pack(side=tk.TOP, fill=tk.X)

        self.intercept_button = ttk.Button(self.proxy_button_frame, text="Intercept", command=self.toggle_intercept)
        self.intercept_button.pack(side=tk.LEFT, padx=5, pady=5)

        self.forward_button = ttk.Button(self.proxy_button_frame, text="Forward", command=self.forward_packet)
        self.forward_button.pack(side=tk.LEFT, padx=5, pady=5)

        self.drop_button = ttk.Button(self.proxy_button_frame, text="Drop", command=self.drop_packet)
        self.drop_button.pack(side=tk.LEFT, padx=5, pady=5)

        # Request List Frame
        self.request_list_frame = tk.LabelFrame(self.proxy_tab, text="Requests", padx=10, pady=10)
        self.request_list_frame.pack(expand=True, fill='both', padx=10, pady=10)

        self.request_listbox = tk.Listbox(self.request_list_frame, height=15, width=80)
        self.request_listbox.pack(side=tk.LEFT, padx=5, pady=5, fill=tk.BOTH, expand=True)

        self.full_requests={}

        self.request_scrollbar = tk.Scrollbar(self.request_list_frame)
        self.request_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.request_listbox.config(yscrollcommand=self.request_scrollbar.set)
        self.request_scrollbar.config(command=self.request_listbox.yview)

        # Bind click event to load full message
        self.request_listbox.bind("<<ListboxSelect>>", self.load_full_message)

        # Full Message Frame
        self.message_frame = tk.LabelFrame(self.proxy_tab, text="Full Message", padx=10, pady=10)
        self.message_frame.pack(fill=tk.X, padx=10, pady=10)

        self.message_text = tk.Text(self.message_frame, height=8, wrap=tk.WORD)
        self.message_text.pack(fill=tk.BOTH, padx=5, pady=5)

        # Logging Frame
        self.log_frame = tk.LabelFrame(self.proxy_tab, text="Log", padx=10, pady=10)
        self.log_frame.pack(fill=tk.X, padx=10, pady=10)

        self.log_text = tk.Text(self.log_frame, height=10, wrap=tk.WORD)
        self.log_text.pack(fill=tk.BOTH, padx=5, pady=5)

    def setup_history_tab(self):
        """Setup the History tab layout and functionality."""
        self.history_text = tk.Text(self.history_tab, wrap=tk.WORD)
        self.history_text.pack(expand=True, fill=tk.BOTH, padx=10, pady=10)

        # Load history content from server
        #self.load_history()

    def load_full_message(self, event):
        """Load full message for the selected request."""
        try:
            selected_index = self.request_listbox.curselection()
            if selected_index:
                display_message_key = self.request_listbox.get(selected_index)
                full_message = self.full_requests.get(display_message_key, "Full message not found.")
                self.message_text.delete(1.0, tk.END)
                self.message_text.insert(tk.END, full_message)

        except Exception as e:
            self.log_message(f"Error loading message: {e}")

    def load_history(self):
        """Load the history file content."""
        try:
            self.history_text.delete(1.0, tk.END)
            with open(HISTORY_PATH, "r") as file:
                self.history_text.insert(tk.END, file.read())
        except Exception as e:
            self.log_message(f"Error loading history: {e}")


    def toggle_intercept(self):
        """Toggles the intercept state."""
        self.intercept_enabled = not self.intercept_enabled
        self.intercept_button.config(text="Intercept")
        if self.intercept_enabled:
            self.intercept_button.config(text="Intercept Off")
            
        else:
            self.intercept_button.config(text="Intercept On")
            self.clear_boxes()
    
    def clear_boxes(self):
        """Clear the request listbox, message box, and log text."""
        self.request_listbox.delete(0, tk.END)  # Clear the request listbox
        self.message_text.delete(1.0, tk.END)  # Clear the full message box
        self.log_text.delete(1.0, tk.END)  # Clear the log text

    def forward_packet(self):
        """Forward the content from the Full Message box."""
        modified_message = self.message_text.get(1.0, tk.END).strip()
        if not modified_message:
            self.log_message("No message to forward.")
            return
        # print the modified message so that to distinguish from the others
        print("\n",modified_message)
        # Verify if the current content is valid
        if not self.Validate_Request(modified_message):
            self.log_message("The modified message is invalid and was not forwarded.")
            return
        
        # Send the modified content
        self.send_response(modified_message)
        self.log_message(f"Forwarded modified message:\n{modified_message}")


    def drop_packet(self):
        selected_index = self.request_listbox.curselection()
        if not selected_index:  # If no item is selected
            self.log_message("No request selected to drop.")
            return
    
        self.send_response("DROP") 
        self.log_message("Drop button pressed. Connection closed.")

    def log_message(self, message):
        self.log_text.insert(tk.END, message + "\n\n")
        self.log_text.yview(tk.END)

    def history_message(self, message):
        messagebox.showinfo("History", "View your request history here.")
        lines = message.splitlines()  # Împarte mesajul în linii
        for line in lines:
            self.history_text.insert(tk.END, line)  # Adaugă fiecare linie în Listbox
        self.history_text.yview(tk.END)  # Derulează la final

    def load_selected_message(self, event):
        try:
            selected_index = self.history_text.curselection()
            if selected_index:
                message = self.history_text.get(selected_index)
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
        self.intercept_button.config(text="Intercept On")

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
            self.intercept_button.config(text="Intercept On")

    def listen_to_requests(self):
        """Listen for incoming requests and handle according to intercept state."""
        with open(PIPE_REQUEST, "r") as request_pipe:
            while True:
                ready, _, _ = select.select([request_pipe], [], [], 1)  # Timeout of 1 second
                if ready:
                    # Aggregate all lines of the request
                    request_lines = []
                    while True:
                        line = request_pipe.readline()
                        if line == "\n" or line == "":  # End of headers
                            break
                        request_lines.append(line)
                    
                    # Join lines into a single request string
                    request = "".join(request_lines)
                   
                    if request:
                        if self.intercept_enabled:
                            print("Inside intercept")
                            # verify if it is a valid request
                            if self.Validate_Request(request) is True:
                                self.log_message(f"Intercepted Request:\n{request}")
                            
                                method, host = self.parse_request(request) 
                                display_message = f"{method} - {host}"
                                
                                # insert only the method - host to the request list
                                self.request_listbox.insert(0, display_message)
                                # insert the entire request into the request list
                                self.full_requests[display_message] = request
                        else: 
                            self.send_response(request)
                    else: continue 

    def Validate_Request(self,request):
        self.valid_methods = {"GET", "POST", "PUT", "DELETE", "PATCH", "OPTIONS", "HEAD", "CONNECT", "TRACE", "UPDATE"}
        if not request or not isinstance(request, str):
            return False 
        first_word = request.split()[0] if request.strip() else ""

        if first_word in self.valid_methods: 
            return True
        return False; 
    

    def parse_request(self, request):
        """Parse HTTP request to extract method and host."""
        method = "UNKNOWN"
        host = "UNKNOWN"
        lines = request.splitlines()
        if lines:
            first_line = lines[0].split()
            if len(first_line) > 0:
                method = first_line[0]
            for line in lines:
                # For printing the entire request strings print(line)
                if line.startswith("Host:"):
                    host = line.split(":", 1)[1].strip()
                    break
        return method, host
    
    
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
