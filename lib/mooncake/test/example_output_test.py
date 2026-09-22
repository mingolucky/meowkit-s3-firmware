import json
import subprocess
import os

# 从 JSON 文件加载测试用例


def load_test_cases(json_file):
    with open(json_file, "r", encoding="utf-8") as file:
        data = json.load(file)
        return data["example_output"]


def run_test_case(case, base_path):
    print(f"正在测试: {case['name']}...")
    executable_path = os.path.join(base_path, case["path"])
    try:
        # 执行可执行文件并捕获输出
        result = subprocess.run(
            executable_path,
            text=True,
            capture_output=True,
            check=True
        )
        output_lines = result.stdout.strip().split("\n")

        # 比对输出
        if output_lines == case["expected_output"]:
            print(f"✅ {case['name']} 测试通过")
        else:
            print(f"❌ {case['name']} 测试失败：输出与期望不一致")
            print("实际输出:")
            for line in output_lines:
                print(line)
            print("\n期望输出:")
            for line in case["expected_output"]:
                print(line)

    except subprocess.CalledProcessError as e:
        print(f"❌ {case['name']} 执行失败：{e}")
    except FileNotFoundError:
        print(f"❌ 找不到可执行文件：{executable_path}")


def main():
    # 当前脚本的目录
    base_path = os.path.dirname(os.path.abspath(__file__))
    json_file = os.path.join(base_path, "test_cases.json")

    test_cases = load_test_cases(json_file)
    for case in test_cases:
        run_test_case(case, base_path)


if __name__ == "__main__":
    main()
