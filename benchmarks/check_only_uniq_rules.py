
def check_only_uniq_rules(rule_lines):
    seen = set()
    for rl in rule_lines:
        rl = rl.strip()
        if rl in seen:
            print("duplicit:" + rl)
        seen.add(rl)

def uniq(rule_lines):
    seen = set()
    for rl in rule_lines:
        rl = rl.strip()
        if rl not in seen:
            print(rl)
        seen.add(rl)

if __name__ == "__main__":
    fn = "../tests/data/acl1_500"
    with open(fn) as f:
        uniq(f.readlines())
        #check_only_uniq_rules(f.readlines())