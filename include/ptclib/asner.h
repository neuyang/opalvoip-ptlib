/*
 * asner.h
 *
 * Abstract Syntax Notation Encoding Rules classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_ASNER_H
#define PTLIB_ASNER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#if P_ASN

// provide options to omit vertain encodings, if needed
#define     P_INCLUDE_PER
#define     P_INCLUDE_BER
#define     P_INCLUDE_XER

class PASN_Stream;
class PBER_Stream;
class PPER_Stream;

#ifdef P_EXPAT
class PXER_Stream;
class PXMLElement;
#else
#undef      P_INCLUDE_XER
#endif


/////////////////////////////////////////////////////////////////////////////

/** Base class for ASN encoding/decoding.
*/
class PASN_Object : public PObject
{
    PCLASSINFO(PASN_Object, PObject);
  public:
    /** Return a string giving the type of the object */
    virtual PString GetTypeAsString() const = 0;

    PINDEX GetObjectLength() const;
    virtual PINDEX GetDataLength() const = 0;
    virtual PBoolean IsPrimitive() const { return true; }

    virtual PBoolean Decode(PASN_Stream &) = 0;
    virtual void Encode(PASN_Stream &) const = 0;

    PBoolean IsExtendable() const { return extendable; }
    void SetExtendable(PBoolean ext = true) { extendable = ext; }

    enum TagClass {
      UniversalTagClass,
      ApplicationTagClass,
      ContextSpecificTagClass,
      PrivateTagClass,
      DefaultTagClass
    };
    TagClass GetTagClass() const { return tagClass; }

    enum UniversalTags {
      InvalidUniversalTag,
      UniversalBoolean,
      UniversalInteger,
      UniversalBitString,
      UniversalOctetString,
      UniversalNull,
      UniversalObjectId,
      UniversalObjectDescriptor,
      UniversalExternalType,
      UniversalReal,
      UniversalEnumeration,
      UniversalEmbeddedPDV,
      UniversalSequence = 16,
      UniversalSet,
      UniversalNumericString,
      UniversalPrintableString,
      UniversalTeletexString,
      UniversalVideotexString,
      UniversalIA5String,
      UniversalUTCTime,
      UniversalGeneralisedTime,
      UniversalGeneralizedTime = UniversalGeneralisedTime,
      UniversalGraphicString,
      UniversalVisibleString,
      UniversalGeneralString,
      UniversalUniversalString,
      UniversalBMPString = 30
    };

    unsigned GetTag() const  { return tag; }
    virtual void SetTag(unsigned newTag, TagClass tagClass = DefaultTagClass);

    enum ConstraintType {
      Unconstrained,
      PartiallyConstrained,
      FixedConstraint,
      ExtendableConstraint
    };

    enum MinimumValueTag { MinimumValue = INT_MIN };
    enum MaximumValueTag { MaximumValue = INT_MAX };
    void SetConstraints(ConstraintType type, int value)
      { SetConstraintBounds(type, value, value); }
    void SetConstraints(ConstraintType, int lower, MaximumValueTag /*upper*/)
      { SetConstraintBounds(PartiallyConstrained, (int)lower, lower < 0 ? INT_MAX : UINT_MAX); }
    void SetConstraints(ConstraintType, MinimumValueTag lower, unsigned upper)
      { SetConstraintBounds(PartiallyConstrained, (int)lower, (unsigned)upper); }
    void SetConstraints(ConstraintType, MinimumValueTag lower, MaximumValueTag upper)
      { SetConstraintBounds(PartiallyConstrained, (int)lower, (unsigned)upper); }
    void SetConstraints(ConstraintType type, int lower, unsigned upper)
      { SetConstraintBounds(type, lower, upper); }

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual void SetCharacterSet(ConstraintType ctype, const char * charSet);
    virtual void SetCharacterSet(ConstraintType ctype, unsigned firstChar, unsigned lastChar);

    static PINDEX GetMaximumArraySize();
    static void SetMaximumArraySize(PINDEX sz);
    static PINDEX GetMaximumStringSize();
    static void SetMaximumStringSize(PINDEX sz);

  protected:
    PASN_Object(unsigned tag, TagClass tagClass, PBoolean extend = false);

    /// PER extension capability
    PBoolean extendable;
    /// BER tag class
    TagClass tagClass;
    /// ASN object tag
    unsigned tag;
};


/** Base class for constrained ASN encoding/decoding.
*/
class PASN_ConstrainedObject : public PASN_Object
{
    PCLASSINFO(PASN_ConstrainedObject, PASN_Object);
  public:
    PBoolean IsConstrained() const { return constraint != Unconstrained; }
    int GetLowerLimit() const { return lowerLimit; }
    unsigned GetUpperLimit() const { return upperLimit; }

    PBoolean ConstrainedLengthDecode(PPER_Stream & strm, unsigned & length);
    void ConstrainedLengthEncode(PPER_Stream & strm, unsigned length) const;

    PBoolean ConstraintEncode(PPER_Stream & strm, unsigned value) const;

  protected:
    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    PASN_ConstrainedObject(unsigned tag, TagClass tagClass);

    ConstraintType constraint;
    int lowerLimit;
    unsigned upperLimit;
};


/** Class for ASN Null type.
*/
class PASN_Null : public PASN_Object
{
    PCLASSINFO(PASN_Null, PASN_Object);
  public:
    PASN_Null(unsigned tag = UniversalNull,
              TagClass tagClass = UniversalTagClass);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;
};


/** Class for ASN Boolean type.
*/
class PASN_Boolean : public PASN_Object
{
    PCLASSINFO(PASN_Boolean, PASN_Object);
  public:
    PASN_Boolean(PBoolean val = false);
    PASN_Boolean(unsigned tag, TagClass tagClass, PBoolean val = false);

    PASN_Boolean & operator=(PBoolean v) { value = v; return *this; }
    operator PBoolean() const { return value; }
    PBoolean GetValue() const { return value; }
    void SetValue(PBoolean v) { value = v; }

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

  protected:
    PBoolean value;
};


/** Class for ASN Integer type.
*/
class PASN_Integer : public PASN_ConstrainedObject
{
    PCLASSINFO(PASN_Integer, PASN_ConstrainedObject);
  public:
    PASN_Integer(unsigned val = 0);
    PASN_Integer(unsigned tag, TagClass tagClass, unsigned val = 0);

    PASN_Integer & operator=(unsigned value);
    operator unsigned() const { return value; }
    unsigned GetValue() const { return value; }
    void SetValue(unsigned v) { operator=(v); }

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

#ifdef P_INCLUDE_PER
    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;
#endif

    PBoolean IsUnsigned() const;

  protected:
    unsigned value;
};

struct PASN_Names{
    const char * name;
    unsigned value; 
};

/** Class for ASN Enumerated type.
*/
class PASN_Enumeration : public PASN_Object
{
    PCLASSINFO(PASN_Enumeration, PASN_Object);
  public:
    PASN_Enumeration(unsigned val = 0);
    PASN_Enumeration(unsigned tag,
                     TagClass tagClass,
                     unsigned nEnums = UINT_MAX,
                     PBoolean extendable = false,
                     unsigned val = 0);
    PASN_Enumeration(unsigned tag,
                     TagClass tagClass,
                     unsigned nEnums,
                     PBoolean extendable,
                     const PASN_Names * nameSpec,
                     unsigned namesCnt,
                     unsigned val = 0);

    PASN_Enumeration & operator=(unsigned v) { value = v; return *this; }
    operator unsigned() const { return value; }
    unsigned GetValue() const { return value; }
    void SetValue(unsigned v) { value = v; }

    unsigned GetMaximum() const { return maxEnumValue; }

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

#ifdef P_INCLUDE_PER
    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;
#endif

#ifdef P_INCLUDE_XER
    virtual PBoolean DecodeXER(PXER_Stream & strm);
    virtual void EncodeXER(PXER_Stream & strm) const;
#endif

    PINDEX GetValueByName(PString name) const;
  protected:
    unsigned maxEnumValue;
    unsigned value;
    const PASN_Names *names;
    unsigned namesCount;   
};


/** Class for ASN floating point type.
*/
class PASN_Real : public PASN_Object
{
    PCLASSINFO(PASN_Real, PASN_Object);
  public:
    PASN_Real(double val = 0);
    PASN_Real(unsigned tag, TagClass tagClass, double val = 0);

    PASN_Real & operator=(double val) { value = val; return *this; }
    operator double() const { return value; }
    double GetValue() const { return value; }
    void SetValue(double v) { value = v; }

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

  protected:
    double value;
};


/** Class for ASN Object Identifier type.
*/
class PASN_ObjectId : public PASN_Object
{
    PCLASSINFO(PASN_ObjectId, PASN_Object);
  public:
    PASN_ObjectId(const char * dotstr = NULL);
    PASN_ObjectId(unsigned tag, TagClass tagClass);

    PASN_ObjectId(const PASN_ObjectId & other);
    PASN_ObjectId & operator=(const PASN_ObjectId & other);

    PASN_ObjectId & operator=(const char * dotstr);
    PASN_ObjectId & operator=(const PString & dotstr);
    PASN_ObjectId & operator=(const PUnsignedArray & numbers);
    void SetValue(const PString & dotstr);
    void SetValue(const PUnsignedArray & numbers) { value = numbers; }
    void SetValue(const unsigned * numbers, PINDEX size);

    bool operator==(const char * dotstr) const;
    bool operator!=(const char * dotstr) const      { return !operator==(dotstr); }
    bool operator==(const PString & dotstr) const   { return  operator==((const char *)dotstr); }
    bool operator!=(const PString & dotstr) const   { return !operator==((const char *)dotstr); }
    bool operator==(const PASN_ObjectId & id) const { return value == id.value; }

    PINDEX GetSize() const { return value.GetSize(); }
    unsigned operator[](PINDEX idx) const { return value[idx]; }
    const PUnsignedArray & GetValue() const { return value; }
    PString AsString() const;

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean CommonDecode(PASN_Stream & strm, unsigned dataLen);
    void CommonEncode(PBYTEArray & eObjId) const;

  protected:
    PUnsignedArray value;
};


/** Class for ASN Bit String type.
*/
class PASN_BitString : public PASN_ConstrainedObject
{
    PCLASSINFO(PASN_BitString, PASN_ConstrainedObject);
  public:
    PASN_BitString(unsigned nBits = 0, const BYTE * buf = NULL);
    PASN_BitString(unsigned tag, TagClass tagClass, unsigned nBits = 0);

    PASN_BitString(const PASN_BitString & other);
    PASN_BitString & operator=(const PASN_BitString & other);

    void SetData(const PBYTEArray & bytes) { SetData(bytes.GetSize()*8, bytes); }
    void SetData(const BYTE * buf, PINDEX size) { SetData(size*8, buf, size); }
    void SetData(unsigned nBits, const PBYTEArray & bytes);
    void SetData(unsigned nBits, const BYTE * buf, PINDEX size = 0);

    const PBYTEArray & GetData() const { return bitData; }
    const BYTE * GetDataPointer() const { return bitData; }

    unsigned GetSize() const { return totalBits; }
    PBoolean SetSize(unsigned nBits);

    bool operator[](PINDEX bit) const;
    void Set(unsigned bit);
    void Clear(unsigned bit);
    void Invert(unsigned bit);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

#ifdef P_INCLUDE_BER
    PBoolean DecodeBER(PBER_Stream & strm, unsigned len);
    void EncodeBER(PBER_Stream & strm) const;
#endif

#ifdef P_INCLUDE_PER
    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;
#endif

    PBoolean DecodeSequenceExtensionBitmap(PPER_Stream & strm);
    void EncodeSequenceExtensionBitmap(PPER_Stream & strm) const;

  protected:
    unsigned totalBits;
    PBYTEArray bitData;
};


/** Class for ASN Octet String type.
*/
class PASN_OctetString : public PASN_ConstrainedObject
{
    PCLASSINFO(PASN_OctetString, PASN_ConstrainedObject);
  public:
    PASN_OctetString(const char * str = NULL, PINDEX size = 0);
    PASN_OctetString(unsigned tag, TagClass tagClass);

    PASN_OctetString(const PASN_OctetString & other);
    PASN_OctetString & operator=(const PASN_OctetString & other);

    PASN_OctetString & operator=(const char * str);
    PASN_OctetString & operator=(const PString & str);
    PASN_OctetString & operator=(const PBYTEArray & arr);
    void SetValue(const char * str) { operator=(str); }
    void SetValue(const PString & str) { operator=(str); }
    void SetValue(const PBYTEArray & arr) { operator=(arr); }
    void SetValue(const BYTE * data, PINDEX len);
    const PBYTEArray & GetValue() const { return value; }
    PBYTEArray & GetWritableValue() { return value; }
    operator const PBYTEArray &() const { return value; }
    operator const BYTE *() const { return value; }
    PString AsString() const;
    BYTE operator[](PINDEX i) const { return value[i]; }
    BYTE & operator[](PINDEX i) { return value[i]; }
    BYTE * GetPointer(PINDEX sz = 0) { return value.GetPointer(sz); }
    PINDEX GetSize() const { return value.GetSize(); }
    PBoolean SetSize(PINDEX newSize);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

#ifdef P_INCLUDE_PER
    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;
#endif

    PBoolean DecodeSubType(PASN_Object &) const;
    void EncodeSubType(const PASN_Object &);

  protected:
    PBYTEArray value;
};


/** Base class for ASN String types.
*/
class PASN_ConstrainedString : public PASN_ConstrainedObject
{
    PCLASSINFO(PASN_ConstrainedString, PASN_ConstrainedObject);
  public:
    PASN_ConstrainedString & operator=(const char * str);
    PASN_ConstrainedString & operator=(const PString & str) { return operator=((const char *)str); }
    operator const PString &() const { return value; }
    const PString & GetValue() const { return value; }
    void SetValue(const char * v) { operator=(v); }
    void SetValue(const PString & v) { operator=(v); }
    char operator[](PINDEX idx) const { return value[idx]; }

    void SetCharacterSet(ConstraintType ctype, const char * charSet);
    void SetCharacterSet(ConstraintType ctype, unsigned firstChar = 0, unsigned lastChar = 255);
    void SetCharacterSet(const char * charSet, PINDEX size, ConstraintType ctype);

    virtual Comparison Compare(const PObject & obj) const;
    virtual void PrintOn(ostream & strm) const;

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

#ifdef P_INCLUDE_BER
    PBoolean DecodeBER(PBER_Stream & strm, unsigned len);
    void EncodeBER(PBER_Stream & strm) const;
#endif

#ifdef P_INCLUDE_PER
    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;
#endif

  protected:
    PASN_ConstrainedString(const char * canonicalSet, PINDEX setSize,
                           unsigned tag, TagClass tagClass);

    PString value;
    PCharArray characterSet;
    const char * canonicalSet;
    PINDEX canonicalSetSize;
    unsigned canonicalSetBits;
    unsigned charSetUnalignedBits;
    unsigned charSetAlignedBits;
};


#define DECLARE_STRING_CLASS(name) \
  class PASN_##name##String : public PASN_ConstrainedString { \
    PCLASSINFO(PASN_##name##String, PASN_ConstrainedString); \
    public: \
      PASN_##name##String(const char * str = NULL); \
      PASN_##name##String(unsigned tag, TagClass tagClass); \
      PASN_##name##String & operator=(const char * str); \
      PASN_##name##String & operator=(const PString & str); \
      virtual PObject * Clone() const; \
      virtual PString GetTypeAsString() const; \
  }

DECLARE_STRING_CLASS(Numeric);
DECLARE_STRING_CLASS(Printable);
DECLARE_STRING_CLASS(Visible);
DECLARE_STRING_CLASS(IA5);
DECLARE_STRING_CLASS(General);


/** Class for ASN BMP (16 bit) String type.
*/
class PASN_BMPString : public PASN_ConstrainedObject
{
    PCLASSINFO(PASN_BMPString, PASN_ConstrainedObject);
  public:
    PASN_BMPString(const char * str = NULL);
    PASN_BMPString(const PWCharArray & wstr);
    PASN_BMPString(unsigned tag, TagClass tagClass);

    PASN_BMPString(const PASN_BMPString & other);
    PASN_BMPString & operator=(const PASN_BMPString & other);

    PASN_BMPString & operator=(const char * v) { return operator=(PString(v).AsUCS2()); }
    PASN_BMPString & operator=(const PString & v) { return operator=(v.AsUCS2()); }
    PASN_BMPString & operator=(const PWCharArray & v);
    operator PString() const { return GetValue(); }
    operator PWCharArray() const { return value; }
    PString GetValue() const { return value; }
    void GetValue(PWCharArray & v) const { v = value; }
    void SetValue(const char * v) { operator=(PString(v).AsUCS2()); }
    void SetValue(const PString & v) { operator=(v.AsUCS2()); }
    void SetValue(const PWCharArray & v) { operator=(v); }
    void SetValue(const PASN_BMPString & v) { operator=(v.value); }
    void SetValueRaw(const PWCharArray & v) { SetValueRaw(v, v.GetSize()); }
    void SetValueRaw(const wchar_t * val, PINDEX len);

    void SetCharacterSet(ConstraintType ctype, const char * charSet);
    void SetCharacterSet(ConstraintType ctype, const PWCharArray & charSet);
    void SetCharacterSet(ConstraintType ctype, unsigned firstChar, unsigned lastChar);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

#ifdef P_INCLUDE_BER
    PBoolean DecodeBER(PBER_Stream & strm, unsigned len);
    void EncodeBER(PBER_Stream & strm) const;
#endif

#ifdef P_INCLUDE_PER
    PBoolean DecodePER(PPER_Stream & strm);
    void EncodePER(PPER_Stream & strm) const;
#endif

  protected:
    void Construct();
    PBoolean IsLegalCharacter(WORD ch);

    PWCharArray value;
    PWCharArray characterSet;
    wchar_t firstChar, lastChar;
    unsigned charSetUnalignedBits;
    unsigned charSetAlignedBits;
};


class PASN_GeneralisedTime : public PASN_VisibleString
{
    PCLASSINFO(PASN_GeneralisedTime, PASN_VisibleString);
  public:
    PASN_GeneralisedTime()
      : PASN_VisibleString(UniversalGeneralisedTime, UniversalTagClass) { }
    PASN_GeneralisedTime(const PTime & time)
      : PASN_VisibleString(UniversalGeneralisedTime, UniversalTagClass) { SetValue(time); }
    PASN_GeneralisedTime(unsigned theTag, TagClass theTagClass)
      : PASN_VisibleString(theTag, theTagClass) { }

    PASN_GeneralisedTime & operator=(const PTime & time);
    void SetValue(const PTime & time) { operator=(time); }
    PTime GetValue() const;
};


class PASN_UniversalTime : public PASN_VisibleString
{
    PCLASSINFO(PASN_UniversalTime, PASN_VisibleString);
  public:
    PASN_UniversalTime()
      : PASN_VisibleString(UniversalUTCTime, UniversalTagClass) { }
    PASN_UniversalTime(const PTime & time)
      : PASN_VisibleString(UniversalUTCTime, UniversalTagClass) { SetValue(time); }
    PASN_UniversalTime(unsigned theTag, TagClass theTagClass)
      : PASN_VisibleString(theTag, theTagClass) { }

    PASN_UniversalTime & operator=(const PTime & time);
    void SetValue(const PTime & time) { operator=(time); }
    PTime GetValue() const;
};


class PASN_Sequence;

/** Class for ASN Choice type.
*/
class PASN_Choice : public PASN_Object
{
    PCLASSINFO(PASN_Choice, PASN_Object);
  public:
    ~PASN_Choice();

    virtual void SetTag(unsigned newTag, TagClass tagClass = DefaultTagClass);
    PString GetTagName() const;
    PASN_Object & GetObject() const;
    PBoolean IsValid() const { return choice != NULL; }

#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9

    operator PASN_Null &() const;
    operator PASN_Boolean &() const;
    operator PASN_Integer &() const;
    operator PASN_Enumeration &() const;
    operator PASN_Real &() const;
    operator PASN_ObjectId &() const;
    operator PASN_BitString &() const;
    operator PASN_OctetString &() const;
    operator PASN_NumericString &() const;
    operator PASN_PrintableString &() const;
    operator PASN_VisibleString &() const;
    operator PASN_IA5String &() const;
    operator PASN_GeneralString &() const;
    operator PASN_BMPString &() const;
    operator PASN_Sequence &() const;

#else

    operator PASN_Null &();
    operator PASN_Boolean &();
    operator PASN_Integer &();
    operator PASN_Enumeration &();
    operator PASN_Real &();
    operator PASN_ObjectId &();
    operator PASN_BitString &();
    operator PASN_OctetString &();
    operator PASN_NumericString &();
    operator PASN_PrintableString &();
    operator PASN_VisibleString &();
    operator PASN_IA5String &();
    operator PASN_GeneralString &();
    operator PASN_BMPString &();
    operator PASN_Sequence &();

    operator const PASN_Null &() const;
    operator const PASN_Boolean &() const;
    operator const PASN_Integer &() const;
    operator const PASN_Enumeration &() const;
    operator const PASN_Real &() const;
    operator const PASN_ObjectId &() const;
    operator const PASN_BitString &() const;
    operator const PASN_OctetString &() const;
    operator const PASN_NumericString &() const;
    operator const PASN_PrintableString &() const;
    operator const PASN_VisibleString &() const;
    operator const PASN_IA5String &() const;
    operator const PASN_GeneralString &() const;
    operator const PASN_BMPString &() const;
    operator const PASN_Sequence &() const;

#endif

    virtual PBoolean CreateObject() = 0;

    virtual Comparison Compare(const PObject & obj) const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean IsPrimitive() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

#ifdef P_INCLUDE_PER
    virtual PBoolean DecodePER(PPER_Stream &);
    virtual void EncodePER(PPER_Stream &) const;
#endif

#ifdef P_INCLUDE_XER
    PBoolean DecodeXER(PXER_Stream &);
    void EncodeXER(PXER_Stream &) const;
#endif

    PASN_Choice & operator=(const PASN_Choice & other);

    PINDEX GetValueByName(PString name) const;
  protected:
    PASN_Choice(unsigned nChoices = 0, PBoolean extend = false);
    PASN_Choice(unsigned tag, TagClass tagClass, unsigned nChoices, PBoolean extend);
    PASN_Choice(unsigned tag, TagClass tagClass, unsigned nChoices, PBoolean extend, const PASN_Names * nameSpec,unsigned namesCnt);

    PASN_Choice(const PASN_Choice & other);

    PBoolean CheckCreate() const;

    unsigned numChoices;
    PASN_Object * choice;
    const PASN_Names *names;
    unsigned namesCount;
};


PARRAY(PASN_ObjectArray, PASN_Object);


/** Class for ASN Sequence type.
*/
class PASN_Sequence : public PASN_Object
{
    PCLASSINFO(PASN_Sequence, PASN_Object);
  public:
    PASN_Sequence(unsigned tag = UniversalSequence,
                  TagClass tagClass = UniversalTagClass,
                  unsigned nOpts = 0, PBoolean extend = false, unsigned nExtend = 0);

    PASN_Sequence(const PASN_Sequence & other);
    PASN_Sequence & operator=(const PASN_Sequence & other);

    PINDEX GetSize() const { return fields.GetSize(); }
    PBoolean SetSize(PINDEX newSize);
    PASN_Object & operator[](PINDEX i) const { return fields[i]; }

    PBoolean HasOptionalField(PINDEX opt) const;
    void IncludeOptionalField(PINDEX opt);
    void RemoveOptionalField(PINDEX opt);

    virtual Comparison Compare(const PObject & obj) const;
    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;

    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean IsPrimitive() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    PBoolean PreambleDecode(PASN_Stream & strm);
    void PreambleEncode(PASN_Stream & strm) const;
    PBoolean KnownExtensionDecode(PASN_Stream & strm, PINDEX fld, PASN_Object & field);
    void KnownExtensionEncode(PASN_Stream & strm, PINDEX fld, const PASN_Object & field) const;
    PBoolean UnknownExtensionsDecode(PASN_Stream & strm);
    void UnknownExtensionsEncode(PASN_Stream & strm) const;

#ifdef P_INCLUDE_BER
    PBoolean PreambleDecodeBER(PBER_Stream & strm);
    void PreambleEncodeBER(PBER_Stream & strm) const;
    PBoolean KnownExtensionDecodeBER(PBER_Stream & strm, PINDEX fld, PASN_Object & field);
    void KnownExtensionEncodeBER(PBER_Stream & strm, PINDEX fld, const PASN_Object & field) const;
    PBoolean UnknownExtensionsDecodeBER(PBER_Stream & strm);
    void UnknownExtensionsEncodeBER(PBER_Stream & strm) const;
#endif

#ifdef P_INCLUDE_PER
    PBoolean PreambleDecodePER(PPER_Stream & strm);
    void PreambleEncodePER(PPER_Stream & strm) const;
    PBoolean KnownExtensionDecodePER(PPER_Stream & strm, PINDEX fld, PASN_Object & field);
    void KnownExtensionEncodePER(PPER_Stream & strm, PINDEX fld, const PASN_Object & field) const;
    PBoolean UnknownExtensionsDecodePER(PPER_Stream & strm);
    void UnknownExtensionsEncodePER(PPER_Stream & strm) const;
#endif

#ifdef P_INCLUDE_XER
    virtual PBoolean PreambleDecodeXER(PXER_Stream & strm);
    virtual void PreambleEncodeXER(PXER_Stream & strm) const;
    virtual PBoolean KnownExtensionDecodeXER(PXER_Stream & strm, PINDEX fld, PASN_Object & field);
    virtual void KnownExtensionEncodeXER(PXER_Stream & strm, PINDEX fld, const PASN_Object & field) const;
    virtual PBoolean UnknownExtensionsDecodeXER(PXER_Stream & strm);
    virtual void UnknownExtensionsEncodeXER(PXER_Stream & strm) const;
#endif

  protected:
    PBoolean NoExtensionsToDecode(PPER_Stream & strm);
    PBoolean NoExtensionsToEncode(PPER_Stream & strm);

    PASN_ObjectArray fields;
    PASN_BitString optionMap;
    int knownExtensions;
    int totalExtensions;
    PASN_BitString extensionMap;
    PINDEX endBasicEncoding;
};


/** Class for ASN Set type.
*/
class PASN_Set : public PASN_Sequence
{
    PCLASSINFO(PASN_Set, PASN_Sequence);
  public:
    PASN_Set(unsigned tag = UniversalSet,
             TagClass tagClass = UniversalTagClass,
             unsigned nOpts = 0, PBoolean extend = false, unsigned nExtend = 0);

    virtual PObject * Clone() const;
    virtual PString GetTypeAsString() const;
};


/** Class for ASN Array type.
*/
class PASN_Array : public PASN_ConstrainedObject
{
    PCLASSINFO(PASN_Array, PASN_ConstrainedObject);
  public:
    PINDEX GetSize() const { return array.GetSize(); }
    PBoolean SetSize(PINDEX newSize);
    PASN_Object & operator[](PINDEX i) const { return array[i]; }
    void Append(PASN_Object * obj) { array.SetAt(array.GetSize(), obj); }
    void RemoveAt(PINDEX i) { array.RemoveAt(i); }
    void RemoveAll() { array.RemoveAll(); }

    virtual Comparison Compare(const PObject & obj) const;
    virtual void PrintOn(ostream & strm) const;

    virtual void SetConstraintBounds(ConstraintType type, int lower, unsigned upper);
    virtual PString GetTypeAsString() const;
    virtual PINDEX GetDataLength() const;
    virtual PBoolean IsPrimitive() const;
    virtual PBoolean Decode(PASN_Stream &);
    virtual void Encode(PASN_Stream &) const;

    virtual PASN_Object * CreateObject() const = 0;

    PASN_Array & operator=(const PASN_Array & other);

  protected:
    PASN_Array(unsigned tag = UniversalSequence,
               TagClass tagClass = UniversalTagClass);

    PASN_Array(const PASN_Array & other);

    PASN_ObjectArray array;
};


/////////////////////////////////////////////////////////////////////////////

/** Base class for ASN decoder/encoder stream.
*/
class PASN_Stream : public PBYTEArray
{
    PCLASSINFO(PASN_Stream, PBYTEArray);
  public:
    PASN_Stream();
    PASN_Stream(const PBYTEArray & bytes);
    PASN_Stream(const BYTE * buf, PINDEX size);

    void PrintOn(ostream & strm) const;

    PINDEX GetPosition() const { return byteOffset; }
    void SetPosition(PINDEX newPos);
    PBoolean IsAtEnd() { return byteOffset >= GetSize(); }
    void ResetDecoder();
    void BeginEncoding();
    void CompleteEncoding();

    virtual PBoolean Read(PChannel & chan) = 0;
    virtual PBoolean Write(PChannel & chan) = 0;

    virtual PBoolean NullDecode(PASN_Null &) = 0;
    virtual void NullEncode(const PASN_Null &) = 0;
    virtual PBoolean BooleanDecode(PASN_Boolean &) = 0;
    virtual void BooleanEncode(const PASN_Boolean &) = 0;
    virtual PBoolean IntegerDecode(PASN_Integer &) = 0;
    virtual void IntegerEncode(const PASN_Integer &) = 0;
    virtual PBoolean EnumerationDecode(PASN_Enumeration &) = 0;
    virtual void EnumerationEncode(const PASN_Enumeration &) = 0;
    virtual PBoolean RealDecode(PASN_Real &) = 0;
    virtual void RealEncode(const PASN_Real &) = 0;
    virtual PBoolean ObjectIdDecode(PASN_ObjectId &) = 0;
    virtual void ObjectIdEncode(const PASN_ObjectId &) = 0;
    virtual PBoolean BitStringDecode(PASN_BitString &) = 0;
    virtual void BitStringEncode(const PASN_BitString &) = 0;
    virtual PBoolean OctetStringDecode(PASN_OctetString &) = 0;
    virtual void OctetStringEncode(const PASN_OctetString &) = 0;
    virtual PBoolean ConstrainedStringDecode(PASN_ConstrainedString &) = 0;
    virtual void ConstrainedStringEncode(const PASN_ConstrainedString &) = 0;
    virtual PBoolean BMPStringDecode(PASN_BMPString &) = 0;
    virtual void BMPStringEncode(const PASN_BMPString &) = 0;
    virtual PBoolean ChoiceDecode(PASN_Choice &) = 0;
    virtual void ChoiceEncode(const PASN_Choice &) = 0;
    virtual PBoolean ArrayDecode(PASN_Array &) = 0;
    virtual void ArrayEncode(const PASN_Array &) = 0;
    virtual PBoolean SequencePreambleDecode(PASN_Sequence &) = 0;
    virtual void SequencePreambleEncode(const PASN_Sequence &) = 0;
    virtual PBoolean SequenceKnownDecode(PASN_Sequence &, PINDEX, PASN_Object &) = 0;
    virtual void SequenceKnownEncode(const PASN_Sequence &, PINDEX, const PASN_Object &) = 0;
    virtual PBoolean SequenceUnknownDecode(PASN_Sequence &) = 0;
    virtual void SequenceUnknownEncode(const PASN_Sequence &) = 0;

    BYTE ByteDecode();
    void ByteEncode(unsigned value);

    unsigned BlockDecode(BYTE * bufptr, unsigned nBytes);
    void BlockEncode(const BYTE * bufptr, PINDEX nBytes);

    void ByteAlign();

  protected:
    PINDEX byteOffset;
    unsigned bitOffset;

  private:
    void Construct();
};

#ifdef  P_INCLUDE_PER
#include "asnper.h"
#endif

#ifdef  P_INCLUDE_BER
#include "asnber.h"
#endif

#ifdef  P_INCLUDE_XER
#include "asnxer.h"
#endif

#endif // P_ASN

#endif // PTLIB_ASNER_H


// End Of File ///////////////////////////////////////////////////////////////
