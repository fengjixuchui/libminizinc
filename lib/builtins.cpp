/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
 *  Main authors:
 *     Guido Tack <guido.tack@monash.edu>
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <minizinc/builtins.hh>
#include <minizinc/ast.hh>
#include <minizinc/eval_par.hh>
#include <minizinc/exception.hh>
#include <minizinc/astiterator.hh>

namespace MiniZinc {
  
  void rb(Model* m, const ASTString& id, const std::vector<Type>& t, 
          FunctionI::builtin_e b) {
    FunctionI* fi = m->matchFn(id,t);
    if (fi) {
      fi->_builtins.e = b;
    } else {
      assert(false); // TODO: is this an error?
    }
  }
  void rb(Model* m, const ASTString& id, const std::vector<Type>& t, 
          FunctionI::builtin_f b) {
    FunctionI* fi = m->matchFn(id,t);
    if (fi) {
      fi->_builtins.f = b;
    } else {
      assert(false); // TODO: is this an error?
    }
  }
  void rb(Model* m, const ASTString& id, const std::vector<Type>& t, 
          FunctionI::builtin_i b) {
    FunctionI* fi = m->matchFn(id,t);
    if (fi) {
      fi->_builtins.i = b;
    } else {
      assert(false); // TODO: is this an error?
    }
  }
  void rb(Model* m, const ASTString& id, const std::vector<Type>& t, 
          FunctionI::builtin_b b) {
    FunctionI* fi = m->matchFn(id,t);
    if (fi) {
      fi->_builtins.b = b;
    } else {
      assert(false); // TODO: is this an error?
    }
  }
  void rb(Model* m, const ASTString& id, const std::vector<Type>& t, 
          FunctionI::builtin_s b) {
    FunctionI* fi = m->matchFn(id,t);
    if (fi) {
      fi->_builtins.s = b;
    } else {
      assert(false); // TODO: is this an error?
    }
  }

  IntVal b_min(ASTExprVec<Expression>& args) {
    switch (args.size()) {
    case 1:
      if (args[0]->_type.isset()) {
        throw EvalError(args[0]->_loc, "sets not supported");
      } else {
        ArrayLit* al = eval_array_lit(args[0]);
        if (al->_v.size()==0)
          throw EvalError(al->_loc, "min on empty array undefined");
        IntVal m = eval_int(al->_v[0]);
        for (unsigned int i=1; i<al->_v.size(); i++)
          m = std::min(m, eval_int(al->_v[i]));
        return m;
      }
    case 2:
      {
        return std::min(eval_int(args[0]),eval_int(args[1]));
      }
    default:
      throw EvalError(Location(), "dynamic type error");
    }
  }

  IntVal b_max(ASTExprVec<Expression>& args) {
    switch (args.size()) {
    case 1:
      if (args[0]->_type.isset()) {
        throw EvalError(args[0]->_loc, "sets not supported");
      } else {
        ArrayLit* al = eval_array_lit(args[0]);
        if (al->_v.size()==0)
          throw EvalError(al->_loc, "max on empty array undefined");
        IntVal m = eval_int(al->_v[0]);
        for (unsigned int i=1; i<al->_v.size(); i++)
          m = std::max(m, eval_int(al->_v[i]));
        return m;
      }
    case 2:
      {
        return std::max(eval_int(args[0]),eval_int(args[1]));
      }
    default:
      throw EvalError(Location(), "dynamic type error");
    }
  }

  typedef std::pair<IntVal,IntVal> Bounds;
  
  Bounds compute_bounds(Expression* e);

  class ComputeBounds : public EVisitor {
  public:
    std::vector<Bounds> _bounds;
    bool enter(Expression* e) {
      if (e->_type.ispar()) {
        if (e->_type.isint()) {
          IntVal v = eval_int(e);
          _bounds.push_back(Bounds(v,v));
        } else {
          throw EvalError(e->_loc, "not yet supported");
        }
        return false;
      } else {
        return true;
      }
    }
    /// Visit integer literal
    void vIntLit(const IntLit& i) {
      _bounds.push_back(Bounds(i._v,i._v));
    }
    /// Visit floating point literal
    void vFloatLit(const FloatLit&) {
      throw EvalError(Location(), "not yet supported");
    }
    /// Visit Boolean literal
    void vBoolLit(const BoolLit&) {
      throw EvalError(Location(), "not yet supported");
    }
    /// Visit set literal
    void vSetLit(const SetLit&) {
      throw EvalError(Location(), "not yet supported");
    }
    /// Visit string literal
    void vStringLit(const StringLit&) {
      throw EvalError(Location(), "not yet supported");
    }
    /// Visit identifier
    void vId(const Id& id) {
      if (id._decl->_ti->_domain) {
        IntSetVal* isv = eval_intset(id._decl->_ti->_domain);
        if (isv->size()==0)
          throw EvalError(id._loc, "Cannot get bounds of "+id._v.str());
        _bounds.push_back(Bounds(isv->min(0),isv->max(isv->size()-1)));
      } else {
        if (id._decl->_e)
          _bounds.push_back(compute_bounds(id._decl->_e));
        else
          throw EvalError(id._loc, "Cannot get bounds of "+id._v.str());
      }
    }
    /// Visit anonymous variable
    void vAnonVar(const AnonVar& v) {
      throw EvalError(v._loc, "Cannot get bounds of anonymous variable");
    }
    /// Visit array literal
    void vArrayLit(const ArrayLit& al) {
      throw EvalError(al._loc, "not yet supported");
    }
    /// Visit array access
    void vArrayAccess(const ArrayAccess& aa) {
      throw EvalError(aa._loc, "not yet supported");
    }
    /// Visit array comprehension
    void vComprehension(const Comprehension& c) {
      throw EvalError(c._loc, "not yet supported");
    }
    /// Visit if-then-else
    void vITE(const ITE& ite) {
      throw EvalError(ite._loc, "not yet supported");
    }
    /// Visit binary operator
    void vBinOp(const BinOp& bo) {
      Bounds b0 = _bounds.back(); _bounds.pop_back();
      Bounds b1 = _bounds.back(); _bounds.pop_back();
      switch (bo.op()) {
      case BOT_PLUS:
        _bounds.push_back(Bounds(b0.first+b1.first,b0.second+b1.second));
        break;
      case BOT_MINUS:
        _bounds.push_back(Bounds(b0.first-b1.second,b0.second-b1.first));
        break;
      case BOT_MULT:
        {
          IntVal x0 = b0.first*b1.first;
          IntVal x1 = b0.first*b1.second;
          IntVal x2 = b0.second*b1.first;
          IntVal x3 = b0.second*b1.second;
          IntVal m = std::min(x0,std::min(x1,std::min(x2,x3)));
          IntVal n = std::max(x0,std::max(x1,std::max(x2,x3)));
          std::cerr << x0 << " " << x1 << " " << x2 << " " << x3 << " " << std::endl;
          _bounds.push_back(Bounds(m,n));
        }
        break;
      case BOT_DIV:
      case BOT_IDIV:
      case BOT_MOD:
      case BOT_LE:
      case BOT_LQ:
      case BOT_GR:
      case BOT_GQ:
      case BOT_EQ:
      case BOT_NQ:
      case BOT_IN:
      case BOT_SUBSET:
      case BOT_SUPERSET:
      case BOT_UNION:
      case BOT_DIFF:
      case BOT_SYMDIFF:
      case BOT_INTERSECT:
      case BOT_PLUSPLUS:
      case BOT_EQUIV:
      case BOT_IMPL:
      case BOT_RIMPL:
      case BOT_OR:
      case BOT_AND:
      case BOT_XOR:
      case BOT_DOTDOT:
        throw EvalError(bo._loc, "not yet supported");
      }
    }
    /// Visit unary operator
    void vUnOp(const UnOp& uo) {
      switch (uo.op()) {
      case UOT_PLUS:
        break;
      case UOT_MINUS:
        _bounds.back().first = -_bounds.back().first;
        _bounds.back().second = -_bounds.back().second;
        break;
      case UOT_NOT:
        throw EvalError(uo._loc, "not yet supported");
      }
    }
    /// Visit call
    void vCall(const Call& c) {
      throw EvalError(c._loc, "not yet supported");
    }
    /// Visit let
    void vLet(const Let& l) {
      throw EvalError(l._loc, "not yet supported");
    }
    /// Visit variable declaration
    void vVarDecl(const VarDecl& vd) {
      throw EvalError(vd._loc, "not yet supported");
    }
    /// Visit annotation
    void vAnnotation(const Annotation& e) {
      throw EvalError(e._loc, "not yet supported");
    }
    /// Visit type inst
    void vTypeInst(const TypeInst& e) {
      throw EvalError(e._loc, "not yet supported");
    }
    /// Visit TIId
    void vTIId(const TIId& e) {
      throw EvalError(e._loc, "not yet supported");
    }
  };

  Bounds compute_bounds(Expression* e) {
    ComputeBounds cb;
    BottomUpIterator<ComputeBounds> cbi(cb);
    cbi.run(e);
    assert(cb._bounds.size()==1);
    return cb._bounds[0];
  }
  
  IntVal lb_varoptint(Expression* e) {
    return compute_bounds(e).first;
  }
  IntVal b_lb_varoptint(ASTExprVec<Expression>& args) {
    if (args.size() != 1)
      throw EvalError(Location(), "dynamic type error");
    return lb_varoptint(args[0]);
  }

  IntVal b_array_lb_int(ASTExprVec<Expression>& args) {
    assert(args.size()==1);
    ArrayLit* al = eval_array_lit(args[0]);
    if (al->_v.size()==0)
      throw EvalError(Location(), "min of empty array undefined");
    IntVal min = lb_varoptint(al->_v[0]);
    for (unsigned int i=1; i<al->_v.size(); i++)
      min = std::min(min, lb_varoptint(al->_v[i]));
    return min;
  }

  IntVal ub_varoptint(Expression* e) {
    return compute_bounds(e).second;
  }
  IntVal b_ub_varoptint(ASTExprVec<Expression>& args) {
    if (args.size() != 1)
      throw EvalError(Location(), "dynamic type error");
    return ub_varoptint(args[0]);
  }

  IntVal b_array_ub_int(ASTExprVec<Expression>& args) {
    assert(args.size()==1);
    ArrayLit* al = eval_array_lit(args[0]);
    if (al->_v.size()==0)
      throw EvalError(Location(), "min of empty array undefined");
    IntVal max = ub_varoptint(al->_v[0]);
    for (unsigned int i=1; i<al->_v.size(); i++)
      max = std::max(max, ub_varoptint(al->_v[i]));
    return max;
  }

  IntVal b_sum(ASTExprVec<Expression>& args) {
    assert(args.size()==1);
    ArrayLit* al = eval_array_lit(args[0]);
    if (al->_v.size()==0)
      return 0;
    IntVal m = 0;
    for (unsigned int i=0; i<al->_v.size(); i++)
      m += eval_int(al->_v[i]);
    return m;
  }

  Expression* b_sum_var(ASTExprVec<Expression>& args) {
    if (args.size()!=1)
      throw EvalError(Location(), "sum needs exactly one argument");
    ArrayLit* al = eval_array_lit(args[0]);
    if (al->_v.size() == 0) {
      return IntLit::a(Location(),0);
    } else {
      Expression* r = al->_v[0];
      for (unsigned int i=1; i<al->_v.size(); i++) {
        r = BinOp::a(Location(),r,BOT_PLUS,al->_v[i]);
        r->_type = Type::varint();
      }
      return r;
    }
  }

  IntSetVal* b_index_set(ASTExprVec<Expression>& args, int i) {
    if (args.size() != 1)
      throw EvalError(Location(), "index_set needs exactly one argument");
    if (args[0]->eid() != Expression::E_ID)
      throw EvalError(Location(), "index_set only supported for identifiers");
    Id* id = args[0]->cast<Id>();
    if (id->_decl == NULL)
      throw EvalError(id->_loc, "undefined identifier");
    if (id->_decl->_ti->_ranges.size() < i)
      throw EvalError(id->_loc, "index_set: wrong dimension");
    if (id->_decl->_ti->_ranges[i-1]->_domain == NULL) {
      ArrayLit* al = eval_array_lit(id);
      if (al->dims() < i)
        throw EvalError(id->_loc, "index_set: wrong dimension");
      return IntSetVal::a(al->min(i-1),al->max(i-1));
    }
    return eval_intset(id->_decl->_ti->_ranges[i-1]->_domain);
  }
  IntSetVal* b_index_set1(ASTExprVec<Expression>& args) {
    return b_index_set(args,1);
  }
  IntSetVal* b_index_set2(ASTExprVec<Expression>& args) {
    return b_index_set(args,2);
  }
  IntSetVal* b_index_set3(ASTExprVec<Expression>& args) {
    return b_index_set(args,3);
  }
  IntSetVal* b_index_set4(ASTExprVec<Expression>& args) {
    return b_index_set(args,4);
  }
  IntSetVal* b_index_set5(ASTExprVec<Expression>& args) {
    return b_index_set(args,5);
  }
  IntSetVal* b_index_set6(ASTExprVec<Expression>& args) {
    return b_index_set(args,6);
  }

  IntVal b_min_parsetint(ASTExprVec<Expression>& args) {
    assert(args.size() == 1);
    IntSetVal* isv = eval_intset(args[0]);
    return isv->min(0);
  }
  IntVal b_max_parsetint(ASTExprVec<Expression>& args) {
    assert(args.size() == 1);
    IntSetVal* isv = eval_intset(args[0]);
    return isv->max(isv->size()-1);
  }

  IntSetVal* b_ub_set(ASTExprVec<Expression>& args) {
    assert(args.size() == 1);
    Expression* e = args[0];
    for (;;) {
      switch (e->eid()) {
      case Expression::E_SETLIT: return eval_intset(e);
      case Expression::E_ID:
        {
          Id* id = e->cast<Id>();
          if (id->_decl==NULL)
            throw EvalError(id->_loc,"undefined identifier");
          if (id->_decl->_e==NULL)
            return eval_intset(id->_decl->_ti->_domain);
          else
            e = id->_decl->_e;
        }
        break;
      default:
        throw EvalError(e->_loc,"invalid argument to ub");
      }
    }
  }

  IntSetVal* b_dom_varint(Expression* e) {
    for (;;) {
      switch (e->eid()) {
      case Expression::E_INTLIT:
        {
          IntVal v = e->cast<IntLit>()->_v;
          return IntSetVal::a(v,v);
        }
      case Expression::E_ID:
        {
          Id* id = e->cast<Id>();
          if (id->_decl==NULL)
            throw EvalError(id->_loc,"undefined identifier");
          if (id->_decl->_e==NULL)
            return eval_intset(id->_decl->_ti->_domain);
          else
            e = id->_decl->_e;
        }
        break;
      default:
        throw EvalError(e->_loc,"invalid argument to dom");
      }
    }
  }
  IntSetVal* b_dom_varint(ASTExprVec<Expression>& args) {
    assert(args.size() == 1);
    return b_dom_varint(args[0]);
  }

  IntSetVal* b_dom_array(ASTExprVec<Expression>& args) {
    assert(args.size() == 1);
    Expression* ae = args[0];
    ArrayLit* al = NULL;
    while (al==NULL) {
      switch (ae->eid()) {
      case Expression::E_ARRAYLIT:
        al = ae->cast<ArrayLit>();
        break;
      case Expression::E_ID:
        {
          Id* id = ae->cast<Id>();
          if (id->_decl==NULL)
            throw EvalError(id->_loc,"undefined identifier");
          if (id->_decl->_e==NULL)
            throw EvalError(id->_loc,"array without initialiser");
          else
            ae = id->_decl->_e;
        }
        break;
      default:
        throw EvalError(ae->_loc,"invalid argument to ub");
      }
    }
    if (al->_v.size()==0)
      return IntSetVal::a();
    IntSetVal* isv = b_dom_varint(al->_v[0]);
    for (unsigned int i=1; i<al->_v.size(); i++) {
      IntSetRanges isr(isv);
      IntSetRanges r(b_dom_varint(al->_v[i]));
      Ranges::Union<IntSetRanges,IntSetRanges> u(isr,r);
      isv = IntSetVal::ai(u);
    }
    return isv;
  }

  ArrayLit* b_arrayXd(ASTExprVec<Expression>& args, int d) {
    ArrayLit* al = eval_array_lit(args[d]);
    std::vector<std::pair<int,int> > dims(d);
    unsigned int dim1d = 1;
    for (int i=0; i<d; i++) {
      IntSetVal* di = eval_intset(args[i]);
      if (di->size() != 1)
        throw EvalError(args[i]->_loc, "arrayXd only defined for ranges");
      dims[i] = std::pair<int,int>(di->min(0),di->max(0));
      dim1d *= dims[i].second-dims[i].first+1;
    }
    if (dim1d != al->_v.size())
      throw EvalError(al->_loc, "mismatch in array dimensions");
    ArrayLit* ret = ArrayLit::a(al->_loc, al->_v, dims);
    ret->_type = al->_type;
    return ret;
  }
  Expression* b_array1d(ASTExprVec<Expression>& args) {
    return b_arrayXd(args,1);
  }
  Expression* b_array2d(ASTExprVec<Expression>& args) {
    return b_arrayXd(args,2);
  }
  Expression* b_array3d(ASTExprVec<Expression>& args) {
    return b_arrayXd(args,3);
  }
  Expression* b_array4d(ASTExprVec<Expression>& args) {
    return b_arrayXd(args,4);
  }
  Expression* b_array5d(ASTExprVec<Expression>& args) {
    return b_arrayXd(args,5);
  }
  Expression* b_array6d(ASTExprVec<Expression>& args) {
    return b_arrayXd(args,6);
  }

  IntVal b_length(ASTExprVec<Expression>& args) {
    ArrayLit* al = eval_array_lit(args[0]);
    return al->_v.size();
  }
  
  IntVal b_bool2int(ASTExprVec<Expression>& args) {
    return eval_bool(args[0]) ? 1 : 0;
  }

  bool b_forall_par(ASTExprVec<Expression>& args) {
    if (args.size()!=1)
      throw EvalError(Location(), "forall needs exactly one argument");
    ArrayLit* al = eval_array_lit(args[0]);
    for (unsigned int i=al->_v.size(); i--;)
      if (!eval_bool(al->_v[i]))
        return false;
    return true;
  }
  Expression* b_forall_var(ASTExprVec<Expression>& args) {
    if (args.size()!=1)
      throw EvalError(Location(), "forall needs exactly one argument");
    ArrayLit* al = eval_array_lit(args[0]);
    if (al->_v.size() == 0) {
      return BoolLit::a(Location(),true);
    } else {
      Expression* r = al->_v[0];
      for (unsigned int i=1; i<al->_v.size(); i++) {
        r = BinOp::a(Location(),r,BOT_AND,al->_v[i]);
        r->_type = Type::varbool();
      }
      return r;
    }
  }
  bool b_exists_par(ASTExprVec<Expression>& args) {
    if (args.size()!=1)
      throw EvalError(Location(), "exists needs exactly one argument");
    ArrayLit* al = eval_array_lit(args[0]);
    for (unsigned int i=al->_v.size(); i--;)
      if (eval_bool(al->_v[i]))
        return true;
    return false;
  }
  Expression* b_exists_var(ASTExprVec<Expression>& args) {
    if (args.size()!=1)
      throw EvalError(Location(), "exists needs exactly one argument");
    ArrayLit* al = eval_array_lit(args[0]);
    if (al->_v.size() == 0) {
      return BoolLit::a(Location(),false);
    } else {
      Expression* r = al->_v[0];
      for (unsigned int i=1; i<al->_v.size(); i++) {
        r = BinOp::a(Location(),r,BOT_OR,al->_v[i]);
        r->_type = Type::varbool();
      }
      return r;
    }
  }

  IntVal b_card(ASTExprVec<Expression>& args) {
    if (args.size()!=1)
      throw EvalError(Location(), "card needs exactly one argument");
    IntSetVal* isv = eval_intset(args[0]);
    IntSetRanges isr(isv);
    return Ranges::size(isr);
  }

  void registerBuiltins(Model* m) {
    
    std::vector<Type> t_intint(2);
    t_intint[0] = Type::parint();
    t_intint[1] = Type::parint();

    std::vector<Type> t_intarray(1);
    t_intarray[0] = Type::parint(-1);
    
    GCLock lock;
    rb(m, ASTString("min"), t_intint, b_min);
    rb(m, ASTString("min"), t_intarray, b_min);
    rb(m, ASTString("max"), t_intint, b_max);
    rb(m, ASTString("max"), t_intarray, b_max);
    rb(m, ASTString("sum"), t_intarray, b_sum);

    {
      std::vector<Type> t(1);
      t[0] = Type::varint(-1);
      rb(m, ASTString("sum"), t, b_sum_var);
    }
    {
      std::vector<Type> t_anyarray1(1);
      t_anyarray1[0] = Type::optvartop(1);
      rb(m, ASTString("index_set"), t_anyarray1, b_index_set1);
    }
    {
      std::vector<Type> t_anyarray2(1);
      t_anyarray2[0] = Type::optvartop(2);
      rb(m, ASTString("index_set_1of2"), t_anyarray2, b_index_set1);
      rb(m, ASTString("index_set_2of2"), t_anyarray2, b_index_set2);
    }
    {
      std::vector<Type> t_anyarray3(1);
      t_anyarray3[0] = Type::optvartop(3);
      rb(m, ASTString("index_set_1of3"), t_anyarray3, b_index_set1);
      rb(m, ASTString("index_set_2of3"), t_anyarray3, b_index_set2);
      rb(m, ASTString("index_set_3of3"), t_anyarray3, b_index_set3);
    }
    {
      std::vector<Type> t_anyarray4(1);
      t_anyarray4[0] = Type::optvartop(4);
      rb(m, ASTString("index_set_1of4"), t_anyarray4, b_index_set1);
      rb(m, ASTString("index_set_2of4"), t_anyarray4, b_index_set2);
      rb(m, ASTString("index_set_3of4"), t_anyarray4, b_index_set3);
      rb(m, ASTString("index_set_4of4"), t_anyarray4, b_index_set4);
    }
    {
      std::vector<Type> t_anyarray5(1);
      t_anyarray5[0] = Type::optvartop(5);
      rb(m, ASTString("index_set_1of5"), t_anyarray5, b_index_set1);
      rb(m, ASTString("index_set_2of5"), t_anyarray5, b_index_set2);
      rb(m, ASTString("index_set_3of5"), t_anyarray5, b_index_set3);
      rb(m, ASTString("index_set_4of5"), t_anyarray5, b_index_set4);
      rb(m, ASTString("index_set_5of5"), t_anyarray5, b_index_set5);
    }
    {
      std::vector<Type> t_anyarray6(1);
      t_anyarray6[0] = Type::optvartop(6);
      rb(m, ASTString("index_set_1of6"), t_anyarray6, b_index_set1);
      rb(m, ASTString("index_set_2of6"), t_anyarray6, b_index_set2);
      rb(m, ASTString("index_set_3of6"), t_anyarray6, b_index_set3);
      rb(m, ASTString("index_set_4of6"), t_anyarray6, b_index_set4);
      rb(m, ASTString("index_set_5of6"), t_anyarray6, b_index_set5);
      rb(m, ASTString("index_set_6of6"), t_anyarray6, b_index_set6);
    }
    {
      std::vector<Type> t_arrayXd(2);
      t_arrayXd[0] = Type::parsetint();
      t_arrayXd[1] = Type::top(-1);
      rb(m, ASTString("array1d"), t_arrayXd, b_array1d);
      t_arrayXd[1] = Type::vartop(-1);
      rb(m, ASTString("array1d"), t_arrayXd, b_array1d);
      t_arrayXd[1] = Type::optvartop(-1);
      rb(m, ASTString("array1d"), t_arrayXd, b_array1d);
    }
    {
      std::vector<Type> t_arrayXd(3);
      t_arrayXd[0] = Type::parsetint();
      t_arrayXd[1] = Type::parsetint();
      t_arrayXd[2] = Type::top(-1);
      rb(m, ASTString("array2d"), t_arrayXd, b_array2d);
      t_arrayXd[2] = Type::vartop(-1);
      rb(m, ASTString("array2d"), t_arrayXd, b_array2d);
      t_arrayXd[2] = Type::optvartop(-1);
      rb(m, ASTString("array2d"), t_arrayXd, b_array2d);
    }
    {
      std::vector<Type> t_arrayXd(4);
      t_arrayXd[0] = Type::parsetint();
      t_arrayXd[1] = Type::parsetint();
      t_arrayXd[2] = Type::parsetint();
      t_arrayXd[3] = Type::top(-1);
      rb(m, ASTString("array3d"), t_arrayXd, b_array3d);
      t_arrayXd[3] = Type::vartop(-1);
      rb(m, ASTString("array3d"), t_arrayXd, b_array3d);
      t_arrayXd[3] = Type::optvartop(-1);
      rb(m, ASTString("array3d"), t_arrayXd, b_array3d);
    }
    {
      std::vector<Type> t_arrayXd(5);
      t_arrayXd[0] = Type::parsetint();
      t_arrayXd[1] = Type::parsetint();
      t_arrayXd[2] = Type::parsetint();
      t_arrayXd[3] = Type::parsetint();
      t_arrayXd[4] = Type::top(-1);
      rb(m, ASTString("array4d"), t_arrayXd, b_array4d);
      t_arrayXd[4] = Type::vartop(-1);
      rb(m, ASTString("array4d"), t_arrayXd, b_array4d);
      t_arrayXd[4] = Type::optvartop(-1);
      rb(m, ASTString("array4d"), t_arrayXd, b_array4d);
    }
    {
      std::vector<Type> t_arrayXd(6);
      t_arrayXd[0] = Type::parsetint();
      t_arrayXd[1] = Type::parsetint();
      t_arrayXd[2] = Type::parsetint();
      t_arrayXd[3] = Type::parsetint();
      t_arrayXd[4] = Type::parsetint();
      t_arrayXd[5] = Type::top(-1);
      rb(m, ASTString("array5d"), t_arrayXd, b_array5d);
      t_arrayXd[5] = Type::vartop(-1);
      rb(m, ASTString("array5d"), t_arrayXd, b_array5d);
      t_arrayXd[5] = Type::optvartop(-1);
      rb(m, ASTString("array5d"), t_arrayXd, b_array5d);
    }
    {
      std::vector<Type> t_arrayXd(7);
      t_arrayXd[0] = Type::parsetint();
      t_arrayXd[1] = Type::parsetint();
      t_arrayXd[2] = Type::parsetint();
      t_arrayXd[3] = Type::parsetint();
      t_arrayXd[4] = Type::parsetint();
      t_arrayXd[5] = Type::parsetint();
      t_arrayXd[6] = Type::top(-1);
      rb(m, ASTString("array6d"), t_arrayXd, b_array6d);
      t_arrayXd[6] = Type::vartop(-1);
      rb(m, ASTString("array6d"), t_arrayXd, b_array6d);
      t_arrayXd[6] = Type::optvartop(-1);
      rb(m, ASTString("array6d"), t_arrayXd, b_array6d);
    }
    {
      std::vector<Type> t_length(1);
      t_length[0] = Type::optvartop(-1);
      rb(m, ASTString("length"), t_length, b_length);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::parbool();
      rb(m, ASTString("bool2int"), t, b_bool2int);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varbool(-1);
      rb(m, ASTString("forall"), t, b_forall_var);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::parbool(-1);
      rb(m, ASTString("forall"), t, b_forall_par);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varbool(-1);
      rb(m, ASTString("exists"), t, b_exists_var);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::parbool(-1);
      rb(m, ASTString("exists"), t, b_exists_par);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varsetint();
      rb(m, ASTString("ub"), t, b_ub_set);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint();
      rb(m, ASTString("dom"), t, b_dom_varint);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint(-1);
      rb(m, ASTString("dom_array"), t, b_dom_array);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::parsetint();
      rb(m, ASTString("min"), t, b_min_parsetint);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::parsetint();
      rb(m, ASTString("max"), t, b_max_parsetint);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint();
      t[0]._ot = Type::OT_OPTIONAL;
      rb(m, ASTString("lb"), t, b_lb_varoptint);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint();
      t[0]._ot = Type::OT_OPTIONAL;
      rb(m, ASTString("ub"), t, b_ub_varoptint);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint();
      rb(m, ASTString("lb"), t, b_lb_varoptint);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint();
      rb(m, ASTString("ub"), t, b_ub_varoptint);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint(-1);
      t[0]._ot = Type::OT_OPTIONAL;
      rb(m, ASTString("lb_array"), t, b_array_lb_int);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::varint(-1);
      t[0]._ot = Type::OT_OPTIONAL;
      rb(m, ASTString("ub_array"), t, b_array_ub_int);
    }
    {
      std::vector<Type> t(1);
      t[0] = Type::parsetint();
      rb(m, ASTString("card"), t, b_card);
    }
  }
  
}


