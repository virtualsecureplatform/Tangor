#!/bin/python3
from Crypto.Cipher import AES

key  = bytes.fromhex("0c0d0e0f08090a0b0405060700010203")[::-1]
data = bytes.fromhex("e0370734313198a2885a308d3243f6a8")[::-1]
print(data)
print(hex(int.from_bytes(data,byteorder='big')))
cipher = AES.new(key, AES.MODE_ECB)
ciphertext = cipher.encrypt(data)
print(ciphertext)
print(hex(int.from_bytes(ciphertext,byteorder='big')))
dec = cipher.decrypt(ciphertext)
print(dec)