import os

def concatenate_files(output_file):
    with open(output_file, 'w') as outfile:
        for root, _, files in os.walk('.'):
            for file in files:
                if file.endswith('.cpp') or file.endswith('.h'):
                    file_path = os.path.join(root, file)
                    with open(file_path, 'r') as infile:
                        outfile.write(f'```\n// {file}\n')
                        outfile.write(infile.read())
                        outfile.write('\n```\n\n')

if __name__ == "__main__":
    concatenate_files('output.md')
