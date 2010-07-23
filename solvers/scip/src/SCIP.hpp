
#ifndef SCIP_H
#define SCIP_H

//#define _DEBUGWRAP 1

#include <vector>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include <scip/type_stat.h>
#include <scip/struct_stat.h>
#include <scip/clock.h>


const int UNSAT     =  0;
const int SAT       =  1;
const int UNKNOWN   =  2;

const int LUBY      =  0;
const int GEOMETRIC =  1;
const int GLUBY     =  2;  




/**
   Expression (Used to encode variables & constraints)
*/
class SCIPSolver;
class SCIP_Expression
{
    
public:

  // scip env
  SCIP *_scip;

  // 
  int _ident;
  int nbj_ident;

  // link to the scip variable
  SCIP_VAR *_var;
  
  double _lower;
  double _upper;
  double _coef;
  bool _continuous;
  
  int get_size();
  int get_max();
  int get_min();

  // link to the set of Boolean scip vars used in the direct encoding
  SCIP_VAR **_encoding;
  virtual void encode(SCIPSolver* solver);

  bool has_been_added() const;

  void initialise(bool c);
  SCIP_Expression();
  virtual ~SCIP_Expression();

  virtual void add(SCIPSolver *solver, bool top_level);
};


class SCIP_FloatVar : public SCIP_Expression 
{
public:
  SCIP_FloatVar();
  SCIP_FloatVar(const double lb, const double ub);
  SCIP_FloatVar(const double lb, const double ub, const int ident);

  double get_value() const;
};


/**
   Array of expressions (for nary constraints)
*/
template<class T>
class SCIPArray
{
private:
  std::vector< T > _array;

public:
  SCIPArray() {}
  virtual ~SCIPArray() {}
  int size() { return _array.size(); }
  void add(T arg) { _array.push_back(arg); }
  T& get_item(const int i) { return _array[i]; }
};

typedef SCIPArray< SCIP_Expression* > SCIPExpArray;
typedef SCIPArray< int > SCIPIntArray;

class SCIP_IntVar : public SCIP_Expression 
{
private:
  SCIPIntArray _values;
  bool _has_holes_in_domain;
public:
  //SCIP_IntVar(const int nval);
  SCIP_IntVar(const int lb, const int ub);
  SCIP_IntVar(const int lb, const int ub, const int ident);
  SCIP_IntVar(SCIPIntArray& values, const int ident);
  virtual void encode(SCIPSolver* solver);
  virtual void add(SCIPSolver* solver, bool top_level);
  int get_value() const;
};


class SCIP_Flow : public SCIP_FloatVar
{
protected:
  int max_val;
  int min_val;
  SCIP_CONS** _constraint;

  //SCIPIntArray _lb;
  //SCIPIntArray _ub;
  //SCIPIntArray _values;
  SCIPExpArray _vars;

  int *card_lb;
  int *card_ub;

public:
  SCIP_Flow(SCIPExpArray& vars);
  SCIP_Flow();
  virtual ~SCIP_Flow();
  virtual void add(SCIPSolver *solver, bool top_level);
  virtual void initbounds();
  void addVar( SCIP_Expression* v );
};

class SCIP_AllDiff : public SCIP_Flow
{
public:
  SCIP_AllDiff(SCIPExpArray& vars);
  SCIP_AllDiff(SCIP_Expression* arg1, SCIP_Expression* arg2);
  virtual ~SCIP_AllDiff();
};

class SCIP_Gcc : public SCIP_Flow
{
public:
  SCIP_Gcc(SCIPExpArray& vars, SCIPIntArray& vals, SCIPIntArray& lb, SCIPIntArray& ub);
  virtual ~SCIP_Gcc();
};

class SCIP_Sum: public SCIP_FloatVar
{
private:
  SCIP_CONS *_constraint;
  int _offset;
  SCIPExpArray _vars;
  SCIPIntArray _weights;
    
public:
  SCIP_Sum(SCIPExpArray& vars, 
	   SCIPIntArray& weights, 
	   const int offset=0);
  SCIP_Sum(SCIP_Expression* arg1, 
	   SCIP_Expression* arg2, 
	   SCIPIntArray& weights,
	   const int offset=0 );
  SCIP_Sum(SCIP_Expression* arg, 
	   SCIPIntArray& weights,
	   const int offset=0 );
  SCIP_Sum();
  void initialise();

  void addVar( SCIP_Expression* v );
  void addWeight( const int w );
  void set_rhs( const int k );

  virtual ~SCIP_Sum();
  virtual void add(SCIPSolver *solver, bool top_level);
};
class SCIP_add : public SCIP_Sum
{
public:
  SCIP_add( SCIP_Expression* arg1, SCIP_Expression* arg2 );
  SCIP_add( SCIP_Expression* arg1, const int arg2 );
  virtual ~SCIP_add();
};
class SCIP_sub : public SCIP_Sum
{
public:
  SCIP_sub( SCIP_Expression* arg1, SCIP_Expression* arg2 );
  SCIP_sub( SCIP_Expression* arg1, const int arg2 );
  virtual ~SCIP_sub();
};


class SCIP_binop: public SCIP_FloatVar
{
protected:
  SCIP_Expression *_vars[2];
  bool _is_proper_coef;
  double _rhs;

public:
  SCIP_binop(SCIP_Expression *var1, SCIP_Expression *var2);
  SCIP_binop(SCIP_Expression *var1, double rhs);
  virtual ~SCIP_binop();

  virtual void add(SCIPSolver *solver, bool top_level) = 0;
};

class SCIP_NoOverlap : public SCIP_binop
{
protected:
  SCIPIntArray _coefs;
public:
  SCIP_NoOverlap(SCIP_Expression *var1, SCIP_Expression *var2, SCIPIntArray &coefs);
  virtual ~SCIP_NoOverlap();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_Precedence : public SCIP_binop
{
protected:
    SCIPIntArray _coefs;
public:
    SCIP_Precedence(SCIP_Expression *var1, SCIP_Expression *var2, SCIPIntArray &coefs);
    virtual ~SCIP_Precedence();
    virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_eq: public SCIP_binop
{
public:
  SCIP_eq(SCIP_Expression *var1, SCIP_Expression *var2);
  SCIP_eq(SCIP_Expression *var1, double rhs);
  virtual ~SCIP_eq();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_ne: public SCIP_binop
{
public:
  SCIP_ne(SCIP_Expression *var1, SCIP_Expression *var2);
  SCIP_ne(SCIP_Expression *var1, double rhs);
  virtual ~SCIP_ne();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_le: public SCIP_binop
{
public:
  SCIP_le(SCIP_Expression *var1, SCIP_Expression *var2);
  SCIP_le(SCIP_Expression *var1, double rhs);
  virtual ~SCIP_le();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_ge: public SCIP_binop
{
public:
  SCIP_ge(SCIP_Expression *var1, SCIP_Expression *var2);
  SCIP_ge(SCIP_Expression *var1, double rhs);
  virtual ~SCIP_ge();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_lt: public SCIP_binop
{
public:
  SCIP_lt(SCIP_Expression *var1, SCIP_Expression *var2);
  SCIP_lt(SCIP_Expression *var1, double rhs);
  virtual ~SCIP_lt();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_gt: public SCIP_binop
{
public:
  SCIP_gt(SCIP_Expression *var1, SCIP_Expression *var2);
  SCIP_gt(SCIP_Expression *var1, double rhs);
  virtual ~SCIP_gt();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_not: public SCIP_binop
{
public:
  SCIP_not(SCIP_Expression *var1);
  virtual ~SCIP_not();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_and: public SCIP_binop
{
public:
  SCIP_and(SCIP_Expression *var1, SCIP_Expression *var2);
  virtual ~SCIP_and();
  virtual void add(SCIPSolver *solver, bool top_level);
};

class SCIP_or: public SCIP_binop
{
public:
  SCIP_or(SCIP_Expression *var1, SCIP_Expression *var2);
  virtual ~SCIP_or();
  virtual void add(SCIPSolver *solver, bool top_level);
} ;

class SCIP_Minimise : public SCIP_eq
{
public:
    SCIP_Minimise(SCIP_Expression *var1);
    virtual ~SCIP_Minimise();
};

class SCIP_Maximise : public SCIP_eq
{
public:
    SCIP_Maximise(SCIP_Expression *var1);
    virtual ~SCIP_Maximise();
};

/**
   The solver itself
*/
class SCIPSolver
{
private:
  std::vector< SCIP_VAR * > _scip_vars;
  std::vector< SCIP_CONS* > _scip_cons;
  std::vector< SCIP_Expression * > _scip_exprs;
  std::vector< SCIPIntArray * > _int_array;
  std::vector< SCIPExpArray * > _var_array;
  
  SCIP * _scip;
  int _verbosity;

public:
  int var_counter;

  SCIPSolver();
  virtual ~SCIPSolver();

  // add an expression, in the case of a tree of expressions,
  // each node of the tree is added separately, depth first.
  void add(SCIP_Expression* arg);
  void add_scip_var (SCIP_VAR * v);
  void add_scip_cons(SCIP_CONS* c);
  
  void add_scip_expr(SCIP_Expression *expr);
  void add_scip_int_array(SCIPIntArray *arr);
  void add_scip_var_array(SCIPExpArray *arr);
  
  SCIP* get_scip();

  // used to initialise search on a given subset of variables
  void initialise(SCIPExpArray& arg);
  // initialise the solver before solving (no more calls to add after this)
  void initialise();

  // solving methods
  int solve();
  int solveAndRestart(const int policy = GEOMETRIC, 
		      const unsigned int base = 32, 
		      const double factor = 1.3333333,
		      const double decay = 0.0);
  int startNewSearch();
  int getNextSolution();
  int sacPreprocess(const int type);
  
  // parameter tuning methods
  void setHeuristic(const char* var_heuristic, const char* val_heuristic, const int rand);
  void setFailureLimit(const int cutoff);  
  void setTimeLimit(const int cutoff);
  void setVerbosity(const int degree);
  void setRandomized(const int degree);
  void setRandomSeed(const int seed);

  // statistics methods
  bool is_sat();
  bool is_unsat();
  void printStatistics();
  int getBacktracks();
  int getNodes();
  int getFailures();
  int getChecks();
  int getPropags();
  double getTime();

};


#endif
