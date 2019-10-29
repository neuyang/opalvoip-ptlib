/*
 * textdata.h
 *
 * Text data stream parser
 *
 * Portable Tools Library
 *
 * Copyright (C) 2019 by Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed
 *                 Blackboard, inc
 */

#ifndef PTLIB_TEXTDATA_H
#define PTLIB_TEXTDATA_H


#include <ptlib/textfile.h>
#include <ptclib/vartype.h>


class PTextDataFormat : public PSmartObject
{
    PCLASSINFO(PTextDataFormat, PSmartObject);
  public:
    PTextDataFormat();
    PTextDataFormat(const PStringArray & headings);

    virtual bool ReadHeadings(PChannel & channel);
    virtual bool WriteHeadings(PChannel & channel);
    virtual bool ReadRecord(PChannel & channel, PStringToString & data);
    virtual bool WriteRecord(PChannel & channel, const PStringToString & data);

    const PStringArray & GetHeadings() const { return m_headings; }

  protected:
    virtual int ReadField(PChannel & channel, PString & field) = 0;
    virtual bool WriteField(PChannel & channel, const PString & field, bool endOfLine) = 0;

    PStringArray m_headings;
};

typedef PSmartPtr<PTextDataFormat> PTextDataFormatPtr;


class PCommaSeparatedVariableFormat : public PTextDataFormat
{
    PCLASSINFO(PCommaSeparatedVariableFormat, PTextDataFormat);
  public:
    PCommaSeparatedVariableFormat();
    PCommaSeparatedVariableFormat(const PStringArray & headings);

  protected:
    virtual int ReadField(PChannel & channel, PString & field);
    virtual bool WriteField(PChannel & channel, const PString & field, bool endOfLine);
};


class PTabDelimitedFormat : public PTextDataFormat
{
    PCLASSINFO(PTabDelimitedFormat, PTextDataFormat);
  public:
    PTabDelimitedFormat();
    PTabDelimitedFormat(const PStringArray & headings);

  protected:
    virtual int ReadField(PChannel & channel, PString & field);
    virtual bool WriteField(PChannel & channel, const PString & field, bool endOfLine);
};


/** Text data file.
    This class reads or writes text data format file,s such as comma separated
    variables or tab delimited files.

    If the file is to be opened for writing, the format must be set, with the
    headings initialised, before the file is opened. A heading line is always
    written on opening.

    If the file is opened for reading, then the class will attempt to
    determine the file format from the file type (e.g. ".csv"), or from the
    data in the first line. Note, if the format cannot be determined then the
    Open() will fail.

    Also for reading, if the headings are preset in the format handler, then
    the headings line is not read.
 */
class PTextDataFile : public PTextFile
{
    PCLASSINFO(PTextDataFile, PTextFile);
  public:
    /**@name Construction */
    //@{
    /** Create a text file object but do not open it. It does not initially
        have a valid file name. However, an attempt to open the file using the
        <code>PFile::Open()</code> function will generate a unique temporary file.
    */
    PTextDataFile(
      PTextDataFormatPtr format = PTextDataFormatPtr()  ///< Format for the text data
    );

    /** Create a text file object with the specified name and open it in the
        specified mode and with the specified options.

        The <code>PChannel::IsOpen()</code> function may be used after object
        construction to determine if the file was successfully opened.
    */
    PTextDataFile(
      const PFilePath & name,    ///< Name of file to open.
      OpenMode mode = ReadOnly,  ///< Mode in which to open the file.
      OpenOptions opts = ModeDefault,    ///< <code>OpenOptions</code> enum# for open operation.
      PTextDataFormatPtr format = PTextDataFormatPtr()  ///< Format for the text data
    );
    //@}

    /** Set the format for the text data file.
        This will fail if the file is already open.
     */
    bool SetFormat(
      const PTextDataFormatPtr & format
    );

    /// Get the text data format for the file.
    PTextDataFormatPtr GetFormat() const { return m_format; }

    /// Read the text data format record
    bool ReadRecord(PStringToString & data);

    // Write the test data format record
    bool WriteRecord(const PStringToString & data);

  protected:
    virtual bool InternalOpen(OpenMode mode, OpenOptions opts, PFileInfo::Permissions permissions);

    PTextDataFormatPtr m_format;
    bool m_formatting;

  private:
    virtual PBoolean Read(void * buf, PINDEX len);
    virtual int ReadChar();
    virtual PBoolean ReadBlock(void * buf,PINDEX len);
    virtual PString ReadString(PINDEX len);
    virtual PBoolean Write(const void * buf, PINDEX len);
    PBoolean WriteChar(int c);
    virtual PBoolean WriteString(const PString & str);
};


#endif // PTLIB_TEXTDATA_H
