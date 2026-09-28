#!/usr/bin/env python

import asyncio
from websockets.server import serve

async def echo(websocket):
    print("handler created", websocket.remote_address[0])
    async for message in websocket:
        print("got message", message)
        await websocket.send(message)

async def main():
    async with serve(echo, "", 8000, subprotocols=["cmdss"]):
        await asyncio.Future()  # run forever

asyncio.run(main())