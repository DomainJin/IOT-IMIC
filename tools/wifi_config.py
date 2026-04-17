#!/usr/bin/env python3
"""
IMIC BLE WiFi Configurator
Cài đặt: pip install bleak
"""

import asyncio
import threading
import tkinter as tk
from tkinter import ttk, messagebox
from bleak import BleakScanner, BleakClient

SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
SSID_UUID    = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
PASS_UUID    = "beb5483e-36e1-4688-b7f5-ea07361b26a9"
STATUS_UUID  = "beb5483e-36e1-4688-b7f5-ea07361b26aa"
CMD_UUID     = "beb5483e-36e1-4688-b7f5-ea07361b26ab"


class App:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("IMIC BLE WiFi Config")
        self.root.resizable(False, False)

        self.devices: dict[str, str] = {}   # label -> address
        self.client: BleakClient | None = None

        self.loop = asyncio.new_event_loop()
        threading.Thread(target=self.loop.run_forever, daemon=True).start()

        self._build_ui()

    # ── UI ─────────────────────────────────────────────────────────────────
    def _build_ui(self):
        pad = {"padx": 10, "pady": 5}

        # ── Scan frame
        frm_scan = ttk.LabelFrame(self.root, text="BLE Devices", padding=8)
        frm_scan.grid(row=0, column=0, **pad, sticky="ew")

        self.lb_devices = tk.Listbox(frm_scan, height=7, width=54,
                                     selectmode=tk.SINGLE, activestyle="dotbox")
        sb = ttk.Scrollbar(frm_scan, command=self.lb_devices.yview)
        self.lb_devices.config(yscrollcommand=sb.set)
        self.lb_devices.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        sb.pack(side=tk.RIGHT, fill=tk.Y)

        # ── Buttons
        frm_btn = ttk.Frame(self.root, padding=4)
        frm_btn.grid(row=1, column=0, sticky="ew", padx=10)

        self.btn_scan = ttk.Button(frm_btn, text="Scan (5s)", command=self.scan)
        self.btn_scan.pack(side=tk.LEFT, padx=4)

        self.btn_connect = ttk.Button(frm_btn, text="Connect", command=self.connect,
                                      state=tk.DISABLED)
        self.btn_connect.pack(side=tk.LEFT, padx=4)

        self.lbl_ble = ttk.Label(frm_btn, text="● Not connected", foreground="gray")
        self.lbl_ble.pack(side=tk.LEFT, padx=8)

        # ── WiFi frame
        frm_wifi = ttk.LabelFrame(self.root, text="WiFi Configuration", padding=8)
        frm_wifi.grid(row=2, column=0, **pad, sticky="ew")

        ttk.Label(frm_wifi, text="SSID:").grid(row=0, column=0, sticky="w", pady=3)
        self.var_ssid = tk.StringVar()
        ttk.Entry(frm_wifi, textvariable=self.var_ssid, width=32).grid(
            row=0, column=1, padx=6, pady=3)

        ttk.Label(frm_wifi, text="Password:").grid(row=1, column=0, sticky="w", pady=3)
        self.var_pass = tk.StringVar()
        self.ent_pass = ttk.Entry(frm_wifi, textvariable=self.var_pass,
                                  width=32, show="●")
        self.ent_pass.grid(row=1, column=1, padx=6, pady=3)

        self.var_show = tk.BooleanVar()
        ttk.Checkbutton(frm_wifi, text="Show", variable=self.var_show,
                        command=self._toggle_pass).grid(row=1, column=2)

        self.btn_send = ttk.Button(frm_wifi, text="Send & Connect WiFi",
                                   command=self.send_wifi, state=tk.DISABLED)
        self.btn_send.grid(row=2, column=0, columnspan=3, pady=6)

        # ── Status bar
        self.var_status = tk.StringVar(value="Ready — click Scan to discover devices")
        ttk.Label(self.root, textvariable=self.var_status,
                  relief="sunken", anchor="w", padding=(6, 2)).grid(
            row=3, column=0, sticky="ew", padx=10, pady=(0, 8))

        self.lb_devices.bind("<<ListboxSelect>>",
                             lambda _: self.btn_connect.config(state=tk.NORMAL))

    def _toggle_pass(self):
        self.ent_pass.config(show="" if self.var_show.get() else "●")

    # ── Helpers ────────────────────────────────────────────────────────────
    def _async(self, coro):
        return asyncio.run_coroutine_threadsafe(coro, self.loop)

    def _status(self, msg: str):
        self.root.after(0, lambda: self.var_status.set(msg))

    def _ble_indicator(self, connected: bool):
        if connected:
            self.root.after(0, lambda: (
                self.lbl_ble.config(text="● Connected", foreground="green"),
                self.btn_send.config(state=tk.NORMAL)))
        else:
            self.root.after(0, lambda: (
                self.lbl_ble.config(text="● Not connected", foreground="gray"),
                self.btn_send.config(state=tk.DISABLED)))

    # ── Scan ───────────────────────────────────────────────────────────────
    def scan(self):
        self.lb_devices.delete(0, tk.END)
        self.devices.clear()
        self.btn_scan.config(state=tk.DISABLED)
        self._status("Scanning for BLE devices (5s)…")
        self._async(self._scan())

    async def _scan(self):
        try:
            # return_adv=True trả về dict {addr: (BLEDevice, AdvertisementData)}
            found = await BleakScanner.discover(timeout=5.0, return_adv=True)
            items = sorted(found.values(),
                           key=lambda x: x[1].rssi if x[1].rssi is not None else -100,
                           reverse=True)
            for device, adv in items:
                name = device.name or adv.local_name or "(no name)"
                rssi = adv.rssi if adv.rssi is not None else "?"
                label = f"{name}  [{device.address}]  {rssi} dBm"
                self.devices[label] = device.address
                self.root.after(0, lambda lbl=label: self.lb_devices.insert(tk.END, lbl))
            self._status(f"Found {len(items)} device(s). Select one and click Connect.")
        except Exception as exc:
            self._status(f"Scan error: {exc}")
        finally:
            self.root.after(0, lambda: self.btn_scan.config(state=tk.NORMAL))

    # ── Connect ────────────────────────────────────────────────────────────
    def connect(self):
        sel = self.lb_devices.curselection()
        if not sel:
            messagebox.showwarning("Select", "Select a device first.")
            return
        label = self.lb_devices.get(sel[0])
        addr  = self.devices[label]
        self._status(f"Connecting to {addr}…")
        self.btn_connect.config(state=tk.DISABLED)
        self._async(self._connect(addr))

    async def _connect(self, addr: str):
        try:
            if self.client and self.client.is_connected:
                await self.client.disconnect()
            self.client = BleakClient(addr, disconnected_callback=self._on_disconnected)
            await self.client.connect()
            await self.client.start_notify(STATUS_UUID, self._on_status_notify)
            self._status(f"Connected: {addr}")
            self._ble_indicator(True)
        except Exception as exc:
            self._status(f"Connection error: {exc}")
            self._ble_indicator(False)
        finally:
            self.root.after(0, lambda: self.btn_connect.config(state=tk.NORMAL))

    def _on_disconnected(self, _client):
        self._status("Device disconnected")
        self._ble_indicator(False)

    def _on_status_notify(self, _sender, data: bytearray):
        msg = data.decode("utf-8", errors="replace")
        self._status(f"Device: {msg}")
        if msg.startswith("ok:"):
            ip = msg[3:]
            self.root.after(0, lambda: messagebox.showinfo(
                "WiFi Connected", f"Device connected!\nIP: {ip}"))
        elif msg.startswith("err:"):
            self.root.after(0, lambda: messagebox.showerror(
                "WiFi Failed", f"Connection failed: {msg[4:]}"))

    # ── Send WiFi ──────────────────────────────────────────────────────────
    def send_wifi(self):
        if not self.client or not self.client.is_connected:
            messagebox.showwarning("Not connected", "Connect to BLE device first.")
            return
        ssid = self.var_ssid.get().strip()
        pwd  = self.var_pass.get()
        if not ssid:
            messagebox.showwarning("SSID", "SSID cannot be empty.")
            return
        self.btn_send.config(state=tk.DISABLED)
        self._status("Sending WiFi credentials…")
        self._async(self._send_wifi(ssid, pwd))

    async def _send_wifi(self, ssid: str, pwd: str):
        try:
            await self.client.write_gatt_char(SSID_UUID, ssid.encode(), response=True)
            await self.client.write_gatt_char(PASS_UUID, pwd.encode(),  response=True)
            await self.client.write_gatt_char(CMD_UUID,  b"1",          response=True)
            self._status("Credentials sent — waiting for device response…")
        except Exception as exc:
            self._status(f"Send error: {exc}")
        finally:
            self.root.after(0, lambda: self.btn_send.config(state=tk.NORMAL))


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
