import sys
import os
#from ajson_lexer import AJSONLexer
#from ajson_parser import AJSONParser


def main():
    print(len(sys.argv))
    # CHECK ARGUMENT NUMBER
    if len(sys.argv) != 2:
    
    not (2 <= len(sys.argv) <= 3):
        raise ValueError(f"INCORRECT ARGUMENT NUMBER:\n"
            f"# PROVIDED: {len(sys.argv)}\n"
            f"# EXPECTED: 2 || 3\n"
            f"# USAGE: python3 ./main.py <path>.ajson [--mode=<mode>]\n"
            f"# - <path>: path to an AJSON file. Examples in ./1-Lex_Yacc/tests\n"
            f"# - <mode>: 0 (default) = lexer & parser || 1 = lexer")
    
    # CHECK MODE
    if len(sys.argv) == 3:
        if sys.argv[2] == "--mode=0":
            mode = 0
        elif sys.argv[2] == "--mode=1":
            mode = 1
        else:
            raise ValueError(f"INCORRECT MODE:\n"
                f"# PROVIDED: {sys.argv[2]}\n"
                f"# EXPECTED: 0 || 1\n"
                f"# USAGE: python3 ./main.py ... [--mode=<mode>]\n"
                f"# - ...\n"
                f"# - <mode>: 0 (default) = lexer & parser || 1 = lexer")
    else:
        mode = 0
    
    # CHECK FILE EXTENSION
    if os.path.splitext(sys.argv[1])[1] != ".ajson":
        raise ValueError(f"INCORRECT FILE EXTENSION:\n"
            f"# PROVIDED: {os.path.splitext(sys.argv[1])[1]}\n"
            f"# EXPECTED: .ajson")

    # OPEN <file_path>.ajson
    try:
        with open(sys.argv[1], 'r', encoding="UTF-8") as file:
            data = file.read()
    except FileNotFoundError:
        raise FileNotFoundError(f"FILE PATH NOT EXIST:\n"
            f"# PROVIDED: {sys.argv[1]}")
    
    if mode == 0:
        # LEXER & PARSER
        parser = AJSONParser()
        output = parser.parse(data)
        if output is None:
            output = f">>> EMPTY AJSON FILE {sys.argv[1]}"
        else:
            output = f">>> AJSON FILE {sys.argv[1]}\n" + output
        print(output)
    else:
        # LEXER
        lexer = AJSONLexer()
        output = lexer.tokenize(data)
        print(output)
    return output
    """


if __name__ == "__main__":
    main()