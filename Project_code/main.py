def read_file(file_path):
    with open(file_path, 'r') as file:
        return file.readlines()

def print_differences(file1_lines, file2_lines):
    differences = [line for line in file1_lines if line not in file2_lines]
    differences += [line for line in file2_lines if line not in file1_lines]
    for diff in differences:
        print(diff.strip())

def main():
    file1_path = 'test1.txt'
    file2_path = 'test2.txt'
    
    file1_lines = read_file(file1_path)
    file2_lines = read_file(file2_path)
    
    print_differences(file1_lines, file2_lines)

if __name__ == "__main__":
    main()