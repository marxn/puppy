#include "parser/expression.h"

using namespace std;


Expression::Expression()
{
        this->lValue   = false;
        this->islambda = false;
}

Expression::~Expression()
{
}

void Expression::SetParentNode(Node * node)
{
        this->ParentNode = node;
}

bool Expression::Check() 
{
        return true;
}

void Expression::LValue(bool flag)
{
        this->lValue = flag;
}

bool Expression::isLValue()
{
        return this->lValue;
}

bool Expression::IsLambdaExp()
{
        return this->islambda;
}

ConstValueExpression::ConstValueExpression()
{
        this->Value = NULL;
}

ConstValueExpression::ConstValueExpression(ConstValue * value)
{
        this->Value = value;
}

ConstValueExpression::~ConstValueExpression()
{
}

ConstValue * ConstValueExpression::Calculate(NodeContext * context)
{
        return Value->DupValue();
}

bool ConstValueExpression::Provision()
{
        return true;
}

BinaryExpression::BinaryExpression(Expression * arg1, Expression * arg2)
{
        this->left  = arg1;
        this->right = arg2;
}

ConstValue * BinaryExpression::Calculate(NodeContext * context)
{
        ConstValue * left_store = this->left->Calculate(context);
        ConstValue * right_store = this->right->Calculate(context);

        ConstValue * result = this->CarryOut(left_store, right_store);

        delete left_store;
        delete right_store;

        return result;
}

bool BinaryExpression::Provision()
{
        left->SetParentNode(this->ParentNode);
        right->SetParentNode(this->ParentNode);

        if(left->Provision()==false)
        {
                return false;
        }
        if(right->Provision()==false)
        {
                return false;
        }

        return true;
}

bool BinaryExpression::Check()
{
        if(left->Check() && right->Check())
        {
                return true;
        }
        return false;
}

KVExpression::KVExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

ConstValue * KVExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        ValueBox * vb = new ValueBox(right_store);
        KVValue * result = new KVValue(left_store, vb);
        vb->Destroy();
        return result;
}

SetExpression::SetExpression(list<Expression*> * exp)
{
        this->ExprList = exp;
}

ConstValue * SetExpression::Calculate(NodeContext * context)
{
        long index = 0;
        MapValue * result = new MapValue;
        list<KVValue*> tobedone;

        list<Expression*>::iterator i;
        for(i = this->ExprList->begin(); i!=this->ExprList->end(); i++)
        {
                ConstValue * value = (*i)->Calculate(context);
                if(value->GetType()==KeyValue)
                {
                        tobedone.push_back(static_cast<KVValue*>(value));
                }
                else
                {
                        IntegerValue * key = new IntegerValue(index);
                        ValueBox * vb = new ValueBox(value);

                        KVValue * kv = new KVValue(key, vb);
                        result->AddKV(kv);

                        vb->Destroy();
                        delete kv;
                        delete key;
                        delete value;
                        index++;
                }
        }

        list<KVValue*>::iterator j;
        for(j = tobedone.begin(); j!=tobedone.end(); j++)
        {
                KVValue * value = *j;
                result->AddKV(value);
                delete value;
        }

        return result;
}

bool SetExpression::Provision()
{
        list<Expression*>::iterator i;
        for(i = this->ExprList->begin(); i!=this->ExprList->end(); i++)
               {
                (*i)->SetParentNode(this->ParentNode);
                       if((*i)->Provision()!=true)
                {
                               return false;
                       }
        }

        return true;
}

bool SetExpression::Check()
{
        list<Expression*>::iterator i;
        for(i = this->ExprList->begin(); i!=this->ExprList->end(); i++)
        {
                if((*i)->Check()!=true)
                {
                        return false;
                }
        }

        return true;
}

VarExpression::VarExpression(string * arg)
{
        this->VarName  = *arg;
        this->lValue   = true;
        this->VarLayer = 0;
}

ValueBox * VarExpression::GetVarRef(NodeContext * context)
{
        Variable * var = context->GetVariable(this->VarLayer, this->VarDef->GetVarIndex());

        if(var==NULL)
        {
                cerr<<"pupp runtime error: cannot find variable:"<<this->VarName<<endl;
                return NULL;
        }

        if(this->ExprsList.size()==0)
        {
                return var->GetVBox();
        }

        ValueBox * result = var->GetVBox();

        for(list<list<Expression*>*>::iterator gitor = this->ExprsList.begin(); gitor!=this->ExprsList.end(); gitor++)
        {
            if(result==NULL)
            {
                    return NULL;
            }
            list<Expression*> * ExprList = *gitor;
            
            if(result->GetVal()->GetType()==Array)
            {
                    if(static_cast<ArrayValue*>(result->GetVal())->GetDimensionNum()!=ExprList->size())
                    {
                            cerr<<"pupp runtime error: wrong dimension variable: "<<var->GetVarName()<<endl;
                            return NULL;
                    }

                    vector<long> desc;
                    
                    for(list<Expression*>::iterator i = ExprList->begin(); i!=ExprList->end(); i++)
                    {
                            ConstValue * value = (*i)->Calculate(context);
                            if(value->GetType()!=Integer)
                            {
                                    cerr<<"pupp runtime error: invalid index for variable: "<<var->GetVarName()<<endl;
                                    delete value;
                                    return NULL;
                            }
                            
                            desc.push_back(static_cast<IntegerValue*>(value)->GetValue());
                            delete value;
                    }

                    ArrayValue * value = static_cast<ArrayValue*>(result->GetVal());
                    result = value->GetElementBox(desc);
            }
            else if(result->GetVal()->GetType()==Map)
            {
                    for(list<Expression*>::iterator i = ExprList->begin(); i!=ExprList->end(); i++)
                    {
                            if(result->GetVal()->GetType()!=Map)
                            {
                                    cerr<<"pupp runtime error: Expect a collection."<<endl;
                                    return NULL;
                            }

                            MapValue * local = static_cast<MapValue*>(result->GetVal());
                            
                            ConstValue * key = (*i)->Calculate(context);

                            result = local->FindByKey(key->toString());

                            if(result==NULL)
                            {
                                    MapValue * value = new MapValue();
                                    ValueBox * vb = new ValueBox(value);
                                    KVValue * kv = new KVValue(key, vb);
                                    local->AddKV(kv);

                                    delete kv;
                                    delete vb;
                                    delete value;
                                    
                                    result = local->FindByKey(key->toString());
                            }
                            
                            delete key;
                    }
            }
            else
            {
                    return NULL;
            }
        }
        
        return result;
}

ConstValue * VarExpression::Calculate(NodeContext * context)
{
        ValueBox * result = this->GetVarRef(context);
        if(result==NULL)
        {
                return new NullValue;
        }

        return result->GetVal()->DupValue();
}

bool VarExpression::Provision()
{
        for(list<list<Expression*>*>::iterator gitor = this->ExprsList.begin(); gitor!=this->ExprsList.end(); gitor++)
        {
                list<Expression*> * ExprList = *gitor;
                
                for(list<Expression*>::iterator i = ExprList->begin(); i!=ExprList->end(); i++)
                {
                    (*i)->SetParentNode(this->ParentNode);
                    if((*i)->Provision()!=true)
                    {
                            return false;
                    }
                }
        }

        return true;
}

bool VarExpression::Check()
{
        unsigned long layer = 0;
        VariableDef * vardef = this->ParentNode->FindVariable(this->VarName, &layer);
        if(vardef==NULL)
        {
                cerr<<"pupp provision error: Variable "<<this->VarName<<" has not been defined"<<endl;
                return false;
        }
        
        this->VarDef = vardef;
        this->VarLayer = layer;

        for(list<list<Expression*>*>::iterator gitor = this->ExprsList.begin(); gitor!=this->ExprsList.end(); gitor++)
        {
                list<Expression*> * ExprList = *gitor;
                
                for(list<Expression*>::iterator i = ExprList->begin(); i!=ExprList->end(); i++)
                {
                    if((*i)->Check()!=true)
                {
                        return false;
                }
                }
        }
        
        return Expression::Check();
}

void VarExpression::AddOffsetExprsList(list<Expression*> * exprlist)
{
        this->ExprsList.push_back(exprlist);
}

FunctionExpression::FunctionExpression(Expression * object, list<Expression*> * exprlist)
{
        this->FuncObj     = object;
        this->ExprList    = exprlist;
        this->ThreadNum   = -1;
        this->ThreadCount = 0;
        pthread_mutex_init(&this->AccessLock, NULL);
}

FunctionExpression::~FunctionExpression()
{
        pthread_mutex_destroy(&this->AccessLock);
}
    
void FunctionExpression::SetThreadNum(long n)
{
        this->ThreadNum = n;
}

class AsynFuncCallContext
{
public:
        AsynFuncCallContext(FunctionNode * FuncNode, NodeContext  * Context):FuncNode(FuncNode), Context(Context)
        {
        }
        ~AsynFuncCallContext()
        {
        }
        int AsynCall()
        {
                pthread_t thid;
                pthread_attr_t attr;

                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                pthread_create(&thid, &attr, ThreadWrapper, this);
                return 0;
        }
        
        static void * ThreadWrapper(void * arg)
        {
                AsynFuncCallContext * ctx = static_cast<AsynFuncCallContext*>(arg);
                
                int rtn = ctx->FuncNode->Run(ctx->Context);
                delete ctx->Context;
                delete ctx;
        }

        FunctionNode * FuncNode;
        NodeContext  * Context;
};

ConstValue * FunctionExpression::Calculate(NodeContext * context)
{
        ConstValue * result = NULL;
        FuncValue * func = static_cast<FuncValue*>(this->FuncObj->Calculate(context));

        if(func==NULL)
        {
                cerr<<"pupp runtime error: Not a qualified function."<<endl;
                return NULL;
        }

        if(func->GetType()==Null)
        {
                cerr<<"pupp runtime error: Function object has not been initialized."<<endl;
                return NULL;
        }

        FunctionNode * funcnode = func->GetFuncNode();

        if(funcnode)
        {
                if(funcnode->GetArgList()->size()!=this->ExprList->size())
                {
                        cerr<<"pupp runtime error: The number of arguments does not match"<<endl;
                        return NULL;
                }

                list<Expression*>::iterator i;
                list<FuncArgDef*>::iterator j;

                for(i = this->ExprList->begin(), j = funcnode->GetArgList()->begin();
                        i!=this->ExprList->end() && j!=funcnode->GetArgList()->end(); i++,j++)
                {
                        if((*j)->isRef())
                        {
                                if((*i)->isLValue()==false)
                                {
                                        cerr<<"pupp runtime error: Cannot pass any expression other than an lvalue to a reference:"<<(*j)->GetName()<<endl;
                                        return NULL;
                                }
                        }
                }

                int rtn = 0;

                NodeContext * new_ctx = new NodeContext;
                new_ctx->AddFrame(funcnode);

                for(i = this->ExprList->begin(), j = funcnode->GetArgList()->begin(); 
                        i!=this->ExprList->end() && j!=funcnode->GetArgList()->end(); i++,j++)
                {
                        if((*j)->isRef())
                        {
                                VarExpression * exp = static_cast<VarExpression*>(*i);
                                ValueBox * vbox = exp->GetVarRef(context);
                                unsigned long index = (*j)->GetArgIndex();
                                Variable * avatar = new_ctx->GetVariable(0, index);

                                if(avatar)
                                {
                                        if(vbox->GetVal()->GetType() != avatar->GetVarType())
                                        {
                                                cerr<<"pupp runtime error: variable type mismatch when passing args to the function."<<endl;
                                                return NULL;
                                        }

                                        delete avatar->GetVBox();
                                        avatar->SetVBox(vbox);
                                        avatar->GetVBox()->IncRefCount();
                                }
                        }
                        else
                        {
                                ConstValue * value = (*i)->Calculate(context);
                                if(value->GetType()!=(*j)->GetType())
                                {
                                        delete new_ctx;
                                        cerr<<"pupp runtime error: data type mismatch when calling a function."<<endl;
                                        return NULL;
                                }

                                Variable * localvar = new_ctx->GetVariable(0, (*j)->GetArgIndex());
                                if(localvar)
                                {
                                        localvar->SetValue(value);
                                }

                                delete value;
                        }
                }

                list<Variable*> * closurevars = func->GetClosureVars();

                if(closurevars!=NULL)
                {
                        //Put closure variables into the new context.
                        new_ctx->ReplaceVariable(funcnode->GetClosureVarStartIndex(), closurevars);
                }
                
                if(this->ThreadNum==-1)
                {
                        //Donot need to launch a new thread. Let's rock!
                        rtn = funcnode->Run(new_ctx);

                        //Check the result of the function.
                        if(rtn==NODE_RET_NEEDRETURN)
                        {
                                if(funcnode->GetRtnType()==Null)
                                {
                                        delete new_ctx;
                                        cerr<<"pupp runtime error: function does not have a return value."<<endl;
                                        return NULL;
                                }
                                
                                result = new_ctx->FunctionRet;
                                if(funcnode->GetRtnType()!=result->GetType())
                                {
                                        ConstValue * ret = ConstValueCast(result, funcnode->GetRtnType());
                                        delete result;
                                        result = ret;
                                }
                        }
                        else
                        {
                                result = new NullValue;
                        }

                        delete new_ctx;
                }
                else if(this->ThreadNum==0)
                {
                        AsynFuncCallContext * asyncallctx = new AsynFuncCallContext(funcnode, new_ctx);        
                        if(asyncallctx)
                        {
                            asyncallctx->AsynCall();
                        }
                               
                        result = new NullValue;
                }
                else
                {
                        pthread_mutex_lock(&this->AccessLock);
                        
                        if(this->ThreadCount < this->ThreadNum)
                        {
                                AsynFuncCallContext * asyncallctx = new AsynFuncCallContext(funcnode, new_ctx);
                                
                                if(asyncallctx)
                                {
                                    this->ThreadCount++;
                                    asyncallctx->AsynCall();
                                }
                        }
                        
                        pthread_mutex_unlock(&this->AccessLock);
                               
                        result = new NullValue;
                }
        }
        else
        {
                cerr<<"pupp runtime error: cannot call a null function"<<endl;
                return NULL;
        }
        
        delete func;
        return result;
}

bool FunctionExpression::Provision()
{
        this->FuncObj->SetParentNode(this->ParentNode);
        if(this->FuncObj->Provision()!=true)
        {
                return false;
        }

        list<Expression*>::iterator i;
        for(i = this->ExprList->begin(); i!=this->ExprList->end(); i++)
        {
                (*i)->SetParentNode(this->ParentNode);
                if((*i)->Provision()!=true)
                {
                        return false;
                }
        }

        return true;
}

bool FunctionExpression::Check()
{
        if(this->FuncObj->Check()!=true)
        {
                return false;
        }

        list<Expression*>::iterator i;

        for(i = this->ExprList->begin(); i!=this->ExprList->end(); i++)
        {
                if((*i)->Check()!=true)
                {
                        return false;
                }
        }

        return Expression::Check();
}

LambdaExpression::LambdaExpression(FunctionNode * node)
{
        this->FuncNode = node;
        this->islambda = true;
}

ConstValue * LambdaExpression::Calculate(NodeContext * context)
{
        list<ClosureVarDesc*> * CopyVars = this->FuncNode->GetCopyList();
        list<Variable *> * cv = new list<Variable *>;

        if(CopyVars!=NULL)
        {
            list<ClosureVarDesc*>::iterator i;
            for(i = CopyVars->begin(); i != CopyVars->end(); i++)
            {
                string varname = (*i)->GetVarName();
                unsigned long layer  = (*i)->GetSrcLayer();
                VariableDef * srcdef = (*i)->GetSrcDef();
                
                Variable * origin = context->GetVariable(layer, srcdef->GetVarIndex());
                if(origin==NULL)
                {
                    cerr<<"pupp runtime error: cannot find variable: "<<varname<<" in the context"<<endl;
                    return NULL;
                }

                if((*i)->IsRef())
                {
                    cv->push_back(origin->CreateVarRef());
                }
                else
                {
                    if(origin->GetVarType()==Func)
                    {
                        ValueBox * vbox = origin->GetVBox();
                        if(vbox==NULL || vbox->GetVal()->GetType()==Null)
                        {
                            return NULL;
                        }
                    }

                    Variable * cl_var = new Variable(origin->GetVarName(), origin->GetVarType());
                    cl_var->SetSource(origin->GetSource());
                    cl_var->SetRef(origin->GetValue());
                    cv->push_back(cl_var);
                }
            }
        }
        FuncValue * ret = new FuncValue(this->FuncNode, cv);

        return ret;
}

bool LambdaExpression::Provision()
{
        this->FuncNode->SetParentNode(this->ParentNode);
        return this->FuncNode->Provision();
}

bool LambdaExpression::Check()
{
        return this->FuncNode->Check();
}


PlusExpression::PlusExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

PlusExpression::~PlusExpression()
{
}

ConstValue * PlusExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::AddOperation(left_store, right_store);
}


SubtractExpression::SubtractExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

SubtractExpression::~SubtractExpression()
{
}

ConstValue * SubtractExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::SubOperation(left_store, right_store);
}



MultiplicationExpression::MultiplicationExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

MultiplicationExpression::~MultiplicationExpression()
{
}

ConstValue * MultiplicationExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::MulOperation(left_store, right_store);
}

DivisionExpression::DivisionExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

DivisionExpression::~DivisionExpression()
{
}

ConstValue * DivisionExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::DivOperation(left_store, right_store);
}


GTExpression::GTExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

GTExpression::~GTExpression()
{
}

ConstValue * GTExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::GTOperation(left_store, right_store);
}

LTExpression::LTExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

LTExpression::~LTExpression()
{
}

ConstValue * LTExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::LTOperation(left_store, right_store);
}

EQExpression::EQExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

EQExpression::~EQExpression()
{
}

ConstValue * EQExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::EQOperation(left_store, right_store);
}

NEQExpression::NEQExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

NEQExpression::~NEQExpression()
{
}

ConstValue * NEQExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::NEQOperation(left_store, right_store);
}


GEExpression::GEExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

GEExpression::~GEExpression()
{
}

ConstValue * GEExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::GEOperation(left_store, right_store);
}

LEExpression::LEExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

LEExpression::~LEExpression()
{
}

ConstValue * LEExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::LEOperation(left_store, right_store);
}

ANDExpression::ANDExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

ANDExpression::~ANDExpression()
{
}

ConstValue * ANDExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::ANDOperation(left_store, right_store);
}

ORExpression::ORExpression(Expression * arg1, Expression * arg2):BinaryExpression(arg1, arg2)
{
}

ORExpression::~ORExpression()
{
}

ConstValue * ORExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::OROperation(left_store, right_store);
}

NOTExpression::NOTExpression(Expression * arg):BinaryExpression(NULL, arg)
{
        this->left = new ConstValueExpression(new BooleanValue(true));
}

NOTExpression::~NOTExpression()
{
}

ConstValue * NOTExpression::CarryOut(ConstValue * left_store, ConstValue * right_store)
{
        return Operation::NOTOperation(right_store);
}
