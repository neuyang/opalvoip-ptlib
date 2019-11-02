#include <ptlib.h>
#include <ptlib/pprocess.h>

#include <ptclib/textdata.h>

class TextDataTest : public PProcess
{
  PCLASSINFO(TextDataTest, PProcess);
 public:
   TextDataTest();
   void Main();
   void ReadTest(const PString & arg);
   void WriteTestFixed(const PString & arg);
   void WriteTestVariable(const PArgList & args);
};

PCREATE_PROCESS(TextDataTest);


TextDataTest::TextDataTest()
  : PProcess("Text Data Test Program", "TextDataTest", 1, 0, AlphaCode, 0)
{
}

void TextDataTest::Main()
{
  PArgList & args = GetArguments();
  if (!args.Parse("w-write.  Write file\n"
                  "f-fields: Heading name(s)")) {
    cerr << args.Usage() << endl;
    return;
  }

  if (args.HasOption('f'))
    WriteTestVariable(args);
  else if (args.HasOption('w')) {
    for (PINDEX i = 0; i < args.GetCount(); ++i)
      WriteTestFixed(args[i]);
  }
  else {
    for (PINDEX i = 0; i < args.GetCount(); ++i)
      ReadTest(args[i]);
  }
}


class TestRecord : public PVarData::Object
{
public:
  P_VAR_DATA_MEMBER(PString, m_first);
  P_VAR_DATA_MEMBER(int, m_second);
  P_VAR_DATA_MEMBER(bool, m_third);
  P_VAR_DATA_MEMBER(PTime, m_fourth);

  TestRecord()
    : m_first("boris")
    , m_second(2)
    , m_third(true)
    , m_fourth(0)
  { }
};


void TextDataTest::ReadTest(const PString & arg)
{
  PTextDataFile file(arg, PFile::ReadOnly);
  if (!file.IsOpen()) {
    cout << "Could not open " << arg << endl;
    return;
  }

  PVarData::Record data;
  while (file.ReadRecord(data)) {
    cout << "Read data ";
    for (PVarData::Record::iterator it = data.begin(); it != data.end(); ++it)
      cout << it->first<< '=' << it->second << ' ';
    cout << endl;
  }
}


void TextDataTest::WriteTestFixed(const PString & arg)
{
  PTextDataFile outfile;
  if (!outfile.Open(arg, PFile::WriteOnly)) {
    cout << "Could not open " << arg << endl;
      return;
  }

  TestRecord test;
  outfile.WriteObject(test);
  test.m_first = PString("natasha");
  test.m_second = 3;
  test.m_third = false;
  test.m_fourth = PTime();
  outfile.WriteObject(test);
  outfile.Close();

  PTextDataFile infile;
  if (!infile.Open(arg, PFile::ReadOnly)) {
    cout << "Could not open " << arg << endl;
    return;
  }
  infile.ReadObject(test);
  cout << "first=" << test.m_first << ", second=" << test.m_second << ", third=" << test.m_third << ", fourth=" << test.m_fourth << endl;
  infile.ReadObject(test);
  cout << "first=" << test.m_first << ", second=" << test.m_second << ", third=" << test.m_third << ", fourth=" << test.m_fourth << endl;
}


void TextDataTest::WriteTestVariable(const PArgList & arg1)
{
}
