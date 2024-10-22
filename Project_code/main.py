# Compare two log files line by line
def compare_logs(transmitter_file, receiver_file):
    try:
        # Open both files
        with open(transmitter_file, 'r') as trans_file, open(receiver_file, 'r') as recv_file:
            line_num = 1
            difference_found = False

            # Read both files line by line
            for trans_line, recv_line in zip(trans_file, recv_file):
                # If the lines differ
                if trans_line.strip() != recv_line.strip():
                    print(f"Difference found at line {line_num}:")
                    print(f"Transmitter: {trans_line.strip()}")
                    print(f"Receiver   : {recv_line.strip()}\n")
                    difference_found = True
                
                line_num += 1
            
            # Check if receiver has more lines than transmitter
            for recv_line in recv_file:
                print(f"Extra line in receiver at line {line_num}: {recv_line.strip()}")
                line_num += 1
                difference_found = True

            # Check if transmitter has more lines than receiver
            for trans_line in trans_file:
                print(f"Extra line in transmitter at line {line_num}: {trans_line.strip()}")
                line_num += 1
                difference_found = True

            if not difference_found:
                print("No differences found. Both files are identical.")

    except FileNotFoundError as e:
        print(f"Error: {e}")

# Paths to the log files
transmitter_file_path = 'logTransmitter.txt'
receiver_file_path = 'logReceiver.txt'

# Call the function to compare the logs
compare_logs(transmitter_file_path, receiver_file_path)
