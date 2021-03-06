#ifndef _CONSTVAL_H_
#define _CONSTVAL_H_

#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

enum DataType
{
        Null = 0, Integer, Decimal, Boolean, String, KeyValue, Map, Array, Func
};

class ConstValue
{
public:
        ConstValue();
        ConstValue(DataType type);
        virtual ~ConstValue();
        virtual ConstValue * DupValue() = 0;
        DataType GetType();
        virtual std::string toString() = 0;
protected:
        DataType Type;
};

class ValueBox
{
public:
        ValueBox();
        ValueBox(ConstValue * value);
        ~ValueBox();
        void IncRefCount();
        void Destroy();
        void SetVal(ConstValue * val);
        ConstValue * GetVal();
        void Clear();
        ValueBox * Dup();
        
private:
        ConstValue * Value;
        unsigned long RefCount;
};

class NullValue: public ConstValue
{
public:
        NullValue();
        ConstValue * DupValue();
        std::string toString();
};

class StringValue: public ConstValue
{
public:
        StringValue();
        StringValue(std::string value);
        ConstValue * DupValue();
        bool LooksLikeInteger();

        bool LooksLikeDecimal();

        std::string toString();

protected:
        std::string Value;
};

class IntegerValue: public ConstValue
{
public:
        IntegerValue(long value);
        IntegerValue(std::string value);
        IntegerValue(IntegerValue& value);
        long GetValue();
        void SetValue(long n);
        ConstValue * DupValue();
        std::string toString();

protected:
        long Value;
};

class DecimalValue: public ConstValue
{
public:
        DecimalValue();
        DecimalValue(std::string valuestr);
        DecimalValue(long value);
        DecimalValue(IntegerValue * value);
        DecimalValue(const DecimalValue& value);
        DecimalValue(DecimalValue * value);
        DecimalValue& operator = (const DecimalValue& value);
        ~DecimalValue();
        ConstValue * DupValue();
        DecimalValue operator + (DecimalValue& element);
        DecimalValue operator - (DecimalValue& element);
        DecimalValue operator * (DecimalValue& element);
        DecimalValue operator / (DecimalValue& element);
        bool operator > (DecimalValue& element);
        bool operator < (DecimalValue& element);
        bool operator == (DecimalValue& element);
        bool operator >= (DecimalValue& element);
        bool operator <= (DecimalValue& element);
        bool operator != (DecimalValue& element);


        std::string toString();
        mpf_t Value;

};

class BooleanValue: public ConstValue
{
public:
        BooleanValue();
        BooleanValue(bool value);
        BooleanValue(std::string value);
        ConstValue * DupValue();

        bool GetValue();
        std::string toString();

protected:
        bool Value;
};

class KVValue: public ConstValue
{
public:
        KVValue();
        KVValue(ConstValue * key, ValueBox* valuebox);
        ~KVValue();
        ConstValue * DupValue();

        ConstValue * GetKey();
        ValueBox * GetValueBox();

        std::string toString();

protected:
        ConstValue * Key;
        ValueBox * Value;
};

class MapValue: public ConstValue
{
public:
        MapValue();
        MapValue(std::map<std::string, ValueBox*>& value);
        ~MapValue();
        void AddKV(KVValue * kv);
        void RemoveKV(std::string key);
        ValueBox * FindByKey(std::string key);
        std::map<std::string, ValueBox*> * GetValue();
        ConstValue * DupValue();

        std::string toString();
protected:
        std::map<std::string, ValueBox*> Value;
};

class DefaultValueFactory
{
public:
        DefaultValueFactory(DataType type);

        ConstValue * GetValue();

private:
        DataType Type;
};

class ArrayValue: public ConstValue
{
public:
        ArrayValue(DataType type, std::vector<long>& desc, long size);
        ~ArrayValue();
        
        ValueBox * GetElementBox(std::vector<long>& keys);
        ConstValue * GetElementValue(std::vector<long>& keys);
        void SetElementValue(std::vector<long>& keys, ConstValue * value);

        void SetValue(ValueBox ** values);
        DataType GetElementType();
        std::string toString();
        ConstValue * DupValue();
        long GetDimensionNum();
private:
        DataType ElementType;
        ValueBox ** Value;
        std::vector<long> Dim;
        long Size;
};

ConstValue * ConstValueCast(ConstValue * value, DataType dst_type);

#endif
