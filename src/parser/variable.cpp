#include "parser/node.h"
#include "parser/variable.h"

using namespace std;

Variable::Variable(string name, DataType type)
{
        this->Source   = NULL;
        this->VBox     = NULL;
        this->VarName  = name;
        this->VarType  = type;
}

Variable::~Variable()
{
        if(this->VBox)
        {
                this->VBox->Destroy();
        }
}

string Variable::GetVarName()
{
        return this->VarName;
}

void Variable::SetSource(VariableDef * def)
{
        this->Source = def;
}

VariableDef * Variable::GetSource()
{
        return this->Source;
}

void Variable::SetVBox(ValueBox * box)
{
        this->VBox = box;
}

ValueBox * Variable::GetVBox()
{
        return this->VBox;
}

void Variable::SetRef(ConstValue * ref)
{
        if(this->VBox==NULL)
        {
                this->VBox = new ValueBox;
        }
                
        this->VBox->SetVal(ref);
}

void Variable::SetValue(ConstValue * value)
{
        if(this->VBox==NULL)
        {
                this->VBox = new ValueBox;
        }

        if(this->VarType==value->GetType())
        {
                ConstValue * val = value->DupValue();
                this->VBox->SetVal(val);
        }
        else
        {
                ConstValue * newvalue = ConstValueCast(value, this->VarType);
                this->VBox->SetVal(newvalue);
        }
}

ConstValue * Variable::GetValue()
{
        ConstValue * value = NULL;

        if(this->VBox==NULL)
        {
                this->VBox = new ValueBox;
                DefaultValueFactory defvalue(this->VarType);
                value = defvalue.GetValue();
                this->VBox->SetVal(value);
        }
        else
        {
                value = this->VBox->GetVal()->DupValue();
        }

        return value;
}

void Variable::SetVarType(DataType type)
{
        this->VarType = type;
}

DataType Variable::GetVarType()
{
        return this->VarType;
}

DataType Variable::GetValueType()
{
        if(this->VBox==NULL)
        {
                this->VBox = new ValueBox;
                DefaultValueFactory defvalue(this->VarType);
                ConstValue * value = defvalue.GetValue();
                this->VBox->SetVal(value);
        }

        return this->VBox->GetVal()->GetType();
}

Variable * Variable::CreateVarRef()
{
        Variable * ret = new Variable(this->VarName, this->VarType);
        ret->SetSource(this->Source);

        if(this->VBox!=NULL)
        {
                this->VBox->IncRefCount();
                ret->SetVBox(this->VBox);
        }
        return ret;
}

VariableDef::VariableDef(string varname)
{
        this->VarName       = varname;
        this->VarType       = Null;
        this->needInstance  = true;
}

Variable * VariableDef::GetInstance()
{
        Variable * ret = new Variable(this->VarName, this->VarType);
        ret->SetSource(this);
        
        DefaultValueFactory fac(this->VarType);
        ConstValue * value = fac.GetValue();
        ret->SetRef(value);

        return ret;
}

void VariableDef::SetVarType(DataType type)
{
        this->VarType = type;
}

DataType VariableDef::GetVarType()
{
        return this->VarType;
}

string VariableDef::GetVarName()
{
        return VarName;
}

void VariableDef::SetAttachedNode(Node * node)
{
        this->AttachedNode = node;
}

Node * VariableDef::GetAttachedNode()
{
        return this->AttachedNode;
}

void VariableDef::SetVarIndex(unsigned long index)
{
        this->VarIndex = index;
}
        
unsigned long VariableDef::GetVarIndex()
{
        return this->VarIndex;
}

bool VariableDef::NeedInstance()
{
        return this->needInstance;
}

void VariableDef::SetNeedInstance(bool b)
{
        this->needInstance = b;
}
