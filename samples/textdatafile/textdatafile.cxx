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
   void WriteTest(const PArgList & args);
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
                  "f-fields: Heading file name")) {
    cerr << args.Usage() << endl;
    return;
  }

  if (args.HasOption('w'))
    WriteTest(args);
  else {
    for (PINDEX i = 0; i < args.GetCount(); ++i)
      ReadTest(args[i]);
  }
}


void TextDataTest::ReadTest(const PString & arg)
{
  PTextDataFile file(arg, PFile::ReadOnly);
  if (!file.IsOpen()) {
    cout << "Could not open " << arg << endl;
    return;
  }

  PStringToString data;
  while (file.ReadRecord(data)) {
    cout << "Read data ";
    for (PStringToString::iterator it = data.begin(); it != data.end(); ++it)
      cout << it->first<< '=' << it->second << ' ';
    cout << endl;
  }
}


void TextDataTest::WriteTest(const PArgList & args)
{
}
