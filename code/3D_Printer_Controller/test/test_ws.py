import websocket
import json

ws = websocket.WebSocket()
ws.connect("ws://*printers_host*:7125/websocket")
print("✅ Připojeno")

# Odeslání testovacího příkazu
ws.send(json.dumps({"method": "printer.objects.query", "params": {"objects": {}}}))

# Přijetí zprávy
print(ws.recv())
ws.close()