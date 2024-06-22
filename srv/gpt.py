import os

def concatenate_files(output_file):
    def apply_rules(file):
        included = False
        # print(f"Initial state for {file}: {'Included' if included else 'Excluded'}")

        if "LICENSE" in file:
            print(f"Rule matched: {file} contains 'LICENSE', excluding and stopping")
            return False

        if file == "semver.sh":
            print(f"Rule matched: {file} is 'semver.sh', excluding and stopping")
            return False

        if file.endswith('.so') or file.endswith('.o'):
            print(f"Rule matched: {file} ends with '.so' or 'o', excluding")
            return False

        if file.endswith('.so') or file.endswith('.lock'):
            print(f"Rule matched: {file} ends with '.so' or 'o', excluding")
            return False

        if file.endswith('.cpp') or file.endswith('.hpp'):
            included = True
            print(f"Rule matched: {file} ends with '.cpp' or '.hpp', including")

        if file.endswith('.sh'):
            included = True
            print(f"Rule matched: {file} ends with '.sh', including")

        if file.endswith('.toml'):
            included = True
            print(f"Rule matched: {file} ends with '.toml', including")

        if file.endswith('.rs'):
            included = True
            print(f"Rule matched: {file} ends with '.rs', including")

        print(f"Final state for {file}: {'Included' if included else 'Excluded'}")
        return included

    with open(output_file, 'w') as outfile:
        for root, dirs, files in os.walk('.'):
            if '.git' in dirs:
                dirs.remove('.git')
            if '.github' in dirs:
                dirs.remove('.github')
            if 'target' in dirs:
                dirs.remove('target')
            if 'benches' in dirs:
                dirs.remove('benches')
            if 'cli' in dirs:
                dirs.remove('cli')
            if 'examples' in dirs:
                dirs.remove('examples')
            if 'fuzz' in dirs:
                dirs.remove('fuzz')
            if 'misc' in dirs:
                dirs.remove('misc')
            if 'scripts' in dirs:
                dirs.remove('scripts')
            if 'test' in dirs:
                dirs.remove('test')
            if 'test_utils' in dirs:
                dirs.remove('test_utils')
            for file in files:
                if apply_rules(file):
                    file_path = os.path.join(root, file)
                    with open(file_path, 'r') as infile:
                        relative_path = os.path.relpath(file_path)
                        outfile.write(f'```\n// {relative_path}\n')
                        outfile.write(infile.read())
                        outfile.write('\n```\n\n')

if __name__ == "__main__":
    concatenate_files('output.md')
