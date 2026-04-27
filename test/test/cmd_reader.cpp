#include "pch.h"
#include "cmd_reader.h"
#include <fstream>
#include <filesystem>

using namespace cppwinrt;

class response_file
{
    const char* resp_file_name = "respfile.txt";
    void write_response_file(const char* input)
    {
        std::ofstream resp_file(resp_file_name);
        if (!resp_file.is_open())
            FAIL("Response file could not be created");
        resp_file << input;
        resp_file.close();
    }

    void remove_response_file()
    {
        std::remove(resp_file_name);
    }

public:
    response_file(const char* input)
    {
        write_response_file(input);
    }

    template<size_t numOptions>
    reader create_reader(size_t const argc, const char* argv[], const option(&options)[numOptions])
    {
        return reader{ argc, argv, options };
    }

    ~response_file()
    {
        remove_response_file();
    }
};

TEST_CASE("cmd_reader")
{
    static constexpr option options[]
    {
        { "input", 1 },
        { "reference", 0 },
        { "output", 0, 1 },
        { "component", 0, 1 },
        { "filter", 0 },
        { "name", 0, 1 },
        { "verbose", 0, 0 },
        { "min_platform", 0, 1 },
        { "max_platform", 0, 1 },
    };

    // input and output
    {
        const char* argv[] = { "progname", "-in", "example_file.in", "-out", "example_file.out" };
        const size_t argc = 5;
        reader args{ argc, argv, options };

        REQUIRE(args.exists("input"));
        REQUIRE(args.value("input") == "example_file.in");
        REQUIRE_FALSE(args.exists("reference"));
        REQUIRE(args.exists("output"));
        REQUIRE(args.value("output") == "example_file.out");
        REQUIRE_FALSE(args.exists("filter"));
        REQUIRE_FALSE(args.exists("name"));
        REQUIRE_FALSE(args.exists("verbose"));
    }

    // response file #1: filename no quotes
    {
        const char* argv[] = { "progname", "@respfile.txt" };
        const size_t argc = _countof(argv);

        response_file rf{ R"(-in example_file.in -out example_file.out)" };
        reader args = rf.create_reader(argc, argv, options);

        REQUIRE(args.exists("input"));
        REQUIRE(args.value("input") == "example_file.in");
        REQUIRE_FALSE(args.exists("reference"));
        REQUIRE(args.exists("output"));
        REQUIRE(args.value("output") == "example_file.out");
        REQUIRE_FALSE(args.exists("filter"));
        REQUIRE_FALSE(args.exists("name"));
        REQUIRE_FALSE(args.exists("verbose"));
    }

    // response file #2: filename with quotes
    {
        const char* argv[] = { "progname", "@respfile.txt" };
        const size_t argc = _countof(argv);

        response_file rf{ R"(-in "example file.in" -out "example file.out")" };
        reader args = rf.create_reader(argc, argv, options);

        REQUIRE(args.exists("input"));
        REQUIRE(args.value("input") == "example file.in");
        REQUIRE_FALSE(args.exists("reference"));
        REQUIRE(args.exists("output"));
        REQUIRE(args.value("output") == "example file.out");
        REQUIRE_FALSE(args.exists("filter"));
        REQUIRE_FALSE(args.exists("name"));
        REQUIRE_FALSE(args.exists("verbose"));
    }

    // response file #3: filename with quote within name
    {
        const char* argv[] = { "progname", "@respfile.txt" };
        const size_t argc = _countof(argv);

        response_file rf{ R"(-in example\"file.in -out example\"file.out)" };
        reader args = rf.create_reader(argc, argv, options);

        REQUIRE(args.exists("input"));
        REQUIRE(args.value("input") == R"(example"file.in)");
        REQUIRE_FALSE(args.exists("reference"));
        REQUIRE(args.exists("output"));
        REQUIRE(args.value("output") == R"(example"file.out)");
        REQUIRE_FALSE(args.exists("filter"));
        REQUIRE_FALSE(args.exists("name"));
        REQUIRE_FALSE(args.exists("verbose"));
    }

    // response file #4: really, really, long path
    {
        const char* argv[] = { "progname", "@respfile.txt" };
        const size_t argc = _countof(argv);
        std::string file_name_in(R"(C:\)");
        std::string file_name_out(R"(C:\)");
        std::string input_str("-in ");

        for (int i = 0; i < 500; i++) {
            file_name_in.append(R"(dirname\)");
            file_name_out.append(R"(dirname\)");
        }

        file_name_in.append("example_file.in");
        file_name_out.append("example_file.out");
        input_str.append(file_name_in).append(" -out ").append(file_name_out);

        response_file rf{ input_str.data() };
        reader args = rf.create_reader(argc, argv, options);

        REQUIRE(args.exists("input"));
        REQUIRE(args.value("input") == file_name_in);
        REQUIRE_FALSE(args.exists("reference"));
        REQUIRE(args.exists("output"));
        REQUIRE(args.value("output") == file_name_out);
        REQUIRE_FALSE(args.exists("filter"));
        REQUIRE_FALSE(args.exists("name"));
        REQUIRE_FALSE(args.exists("verbose"));
    }

    // min/max platform switches
    {
        const char* argv[] = { "progname", "-in", "example_file.in", "-min_platform", "min_platform.xml", "-max_platform", "max_platform.xml" };
        const size_t argc = _countof(argv);
        reader args{ argc, argv, options };

        REQUIRE(args.exists("min_platform"));
        REQUIRE(args.value("min_platform") == "min_platform.xml");
        REQUIRE(args.exists("max_platform"));
        REQUIRE(args.value("max_platform") == "max_platform.xml");
    }

    // load platform contract map
    {
        auto xml_path = std::filesystem::temp_directory_path() / "cppwinrt_test_platform_contracts.xml";
        std::ofstream xml_file(xml_path);
        REQUIRE(xml_file.is_open());

        xml_file << R"(<?xml version="1.0" encoding="utf-8"?>
<ApplicationPlatform name="UAP" friendlyName="Windows 11, version 24H2" version="10.0.26100.0">
    <ContainedApiContracts>
        <ApiContract name="FooContract" version="1.0.0.0"/>
        <ApiContract name="Windows.Foundation.UniversalApiContract" version="15.0.0.0"/>
    </ContainedApiContracts>
</ApplicationPlatform>)";
        xml_file.close();

        auto contracts = load_platform_contracts(xml_path);

        REQUIRE(contracts.size() == 2);
        REQUIRE(contracts.at("FooContract") == 0x00010000);
        REQUIRE(contracts.at("Windows.Foundation.UniversalApiContract") == 0x000F0000);

        std::filesystem::remove(xml_path);
    }

    // load platform contract map: dotted versions encode major/minor
    {
        auto xml_path = std::filesystem::temp_directory_path() / "cppwinrt_test_platform_contracts_minor.xml";
        std::ofstream xml_file(xml_path);
        REQUIRE(xml_file.is_open());

        xml_file << R"(<?xml version="1.0" encoding="utf-8"?>
<ApplicationPlatform name="UAP" friendlyName="Windows" version="10.0.0.0">
    <ContainedApiContracts>
        <ApiContract name="ContractWithMinor" version="3.5.0.0"/>
    </ContainedApiContracts>
</ApplicationPlatform>)";
        xml_file.close();

        auto contracts = load_platform_contracts(xml_path);

        REQUIRE(contracts.size() == 1);
        REQUIRE(contracts.at("ContractWithMinor") == 0x00030005);

        std::filesystem::remove(xml_path);
    }

    // load platform contract map: invalid path throws
    {
        auto xml_path = std::filesystem::temp_directory_path() / "cppwinrt_does_not_exist.xml";
        REQUIRE_THROWS_AS(load_platform_contracts(xml_path), std::invalid_argument);
    }
}
