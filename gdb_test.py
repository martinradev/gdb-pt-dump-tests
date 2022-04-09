import gdb
import tempfile

class Get_Message(gdb.Command):
    def __init__(self):
        self.line = 0
        super(Get_Message, self).__init__("get_message", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        msg = ""
        vidmem_addr = 0xb8000
        while True:
            res = gdb.execute(f"monitor xp /xb {vidmem_addr}", to_string = True)
            byte = int(res.split(" ")[1], 16)
            if byte == ord('~'):
                break
            msg += chr(byte)
            vidmem_addr += 2
        print(msg)
        return msg

class Next_Test(gdb.Command):
    def __init__(self):
        super(Next_Test, self).__init__("next_test", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        gdb.execute("set *(unsigned char*)0x13370 = 0x1")

class Run_Tests(gdb.Command):
    def __init__(self):
        super(Run_Tests, self).__init__("run_tests", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        res = gdb.execute("get_message", to_string=True).strip()
        print(res.encode("ascii"))
        assert(res == "ENTER")
        gdb.execute(f"awatch *0x13378")
        test_id = 0
        num_suc = 0
        num_fail = 0
        while True:
            gdb.execute("next_test")
            gdb.execute("continue")
            test_name = gdb.execute("get_message", to_string=True).strip()
            if test_name == "DONE":
                break
            print(f"Running test ({test_id}): {test_name}")

            full_test_name = f"pt_test_{test_name}"
            if test_name.startswith("SEARCH_STR"):
                keyword = "abracadabra"
                gdb.execute(f"pt -after 0x10000 -ss {keyword} -o /tmp/{full_test_name}")
            elif test_name.startswith("SEARCH_8b"):
                keyword = test_name.split("_")[-1]
                gdb.execute(f"pt -after 0x10000 -s8 0x11aa22bb33ccdd55 -o /tmp/{full_test_name}")
            elif test_name.startswith("SEARCH_4b"):
                keyword = test_name.split("_")[-1]
                gdb.execute(f"pt -after 0x10000 -s4 0x8833aacc -o /tmp/{full_test_name}")
            else:
                gdb.execute(f"pt -o /tmp/{full_test_name}")

            test_pass = False
            try:
                f1 = open(f"./golden_results/{full_test_name}", "rb")
                golden_data = f1.read()
                f1.close()

                try:
                    f2 = open(f"/tmp/{full_test_name}", "rb")
                    result_data = f2.read()
                    f2.close()
                    test_pass = (golden_data == result_data)
                except Exception as e:
                    print(f"Couldn't get temporary result file: {full_test_name}. Error {e}")

            except Exception as e:
                print(f"Couldn't get golden file: {full_test_name}. Error {e}")

            if test_pass == True:
                print(f"Test {full_test_name} succeeded")
                num_suc += 1
            else:
                print(f"Test {full_test_name} failed")
                num_fail += 1
        
            test_id += 1
            gdb.execute("next_test")

        print(f"Tests done.\n\tSuccess: {num_suc}/{num_suc+num_fail}\n\tFail: {num_fail}")
        gdb.execute("q")
            
        
Next_Test()
Run_Tests()
Get_Message()
