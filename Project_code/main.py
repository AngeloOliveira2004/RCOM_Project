def xor_hex_bytes(hex_list):
    # Convert the hex strings to integers and XOR them
    xor_result = 0
    for hex_byte in hex_list:
        xor_result ^= int(hex_byte, 16)
    
    # Convert the result back to hex and format it like the input list
    result_list = [f"0x{xor_result:02X}"]
    return result_list

# Input list of hex values in the specified format
hex_values = [
    0x00, 0x2A, "0xD8", "0x01", "0x0C", "0x70", "0x65", "0x6E",
    "0x67", "0x75", "0x69", "0x6E", "0x2E", "0x67", "0x69", "0x66",
    "0x2E", "0x67", "0x69", "0x66"
]

# Perform the XOR operation
result = xor_hex_bytes(hex_values)

# Print the result in the specified format
print(" ".join(result))
