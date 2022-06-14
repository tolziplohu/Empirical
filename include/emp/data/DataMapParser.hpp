/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2021-2022.
 *
 *  @file DataMapParser.hpp
 *  @brief Useful functions for working with DataMaps and AnnotatedTypes.
 *  @note Status: ALPHA
 *
 *  Developer TODO:
 *  - Setup operator RegEx to be built dynamically
 *  - Setup LVALUES as a type, and allow assignment
 *  - Allow types other than Datum (string and double)?
 */

#ifndef EMP_DATA_DATAMAPPARSER_HPP_INCLUDE
#define EMP_DATA_DATAMAPPARSER_HPP_INCLUDE

#include <cmath>
#include <string>

#include "../base/notify.hpp"
#include "../compiler/Lexer.hpp"
#include "../compiler/regex_utils.hpp"
#include "../data/Datum.hpp"
#include "../math/Random.hpp"

#include "AnnotatedType.hpp"
#include "DataMap.hpp"

namespace emp {
  class DataMapParser {

    using value_fun_t = std::function<emp::Datum(const emp::DataMap &)>;
    using pos_t = emp::TokenStream::Iterator;

    static constexpr const bool verbose = false;

    class DataMapLexer : public emp::Lexer {
    private:
      int token_identifier;   ///< Token id for identifiers
      int token_number;       ///< Token id for literal numbers
      int token_string;       ///< Token id for literal strings
      int token_char;         ///< Token id for literal characters
      int token_external;     ///< Token id for an external value that was passed in
      int token_symbol;       ///< Token id for other symbols

    public:
      DataMapLexer() {
        // Whitespace and comments should always be dismissed (top priority)
        IgnoreToken("Whitespace", "[ \t\n\r]+");
        IgnoreToken("//-Comments", "//.*");
        IgnoreToken("/*...*/-Comments", "/[*]([^*]|([*]+[^*/]))*[*]+/");

        // Meaningful tokens have next priority.

        // An identifier must begin with a letter, underscore, or dot, and followed by
        // more of the same OR numbers or brackets.
        token_identifier = AddToken("Identifier", "[a-zA-Z_.][a-zA-Z0-9_.[\\]]*");

        // A literal number must begin with a digit; it can have any number of digits in it and
        // optionally a decimal point.
        token_number = AddToken("Literal Number", "[0-9]+(\\.[0-9]+)?");

        // A string must begin and end with a quote and can have an escaped quote in the middle.
        token_string = AddToken("Literal String", "\\\"([^\"\\\\]|\\\\.)*\\\"");

        // A literal char must begin and end with a single quote.  It will always be treated as
        // its ascii value.
        token_char = AddToken("Literal Character", "'([^'\n\\\\]|\\\\.)+'");

        // An external value that was passed in will be a dollar sign ('$') followed by the
        // position of the value to be used (e.g., '$3').
        token_external = AddToken("External Value", "[$][0-9]+");

        // Symbols should have least priority.  They include any solitary character not listed
        // above, or pre-specified multi-character groups.
        token_symbol = AddToken("Symbol", ".|\"==\"|\"!=\"|\"<=\"|\">=\"|\"~==\"|\"~!=\"|\"~<\"|\"~>\"|\"~<=\"|\"~>=\"|\"&&\"|\"||\"|\"**\"|\"%%\"");
      }

      bool IsID(const emp::Token & token) const noexcept { return token.token_id == token_identifier; }
      bool IsNumber(const emp::Token & token) const noexcept { return token.token_id == token_number; }
      bool IsString(const emp::Token & token) const noexcept { return token.token_id == token_string; }
      bool IsChar(const emp::Token & token) const noexcept { return token.token_id == token_char; }
      bool IsExternal(const emp::Token & token) const noexcept { return token.token_id == token_external; }
      bool IsSymbol(const emp::Token & token) const noexcept { return token.token_id == token_symbol; }
    };

    struct ValueType {
      enum type_t { ERROR=0, VALUE, FUNCTION };

      type_t type;
      emp::Datum value;
      value_fun_t fun;

      ValueType() : type(ERROR) {}
      ValueType(const ValueType &) = default;
      ValueType(double in_val) : type(VALUE), value(in_val) { }
      ValueType(std::string in_val) : type(VALUE), value(in_val) { }
      ValueType(emp::Datum in_val) : type(VALUE), value(in_val) { }
      ValueType(value_fun_t in_fun) : type(FUNCTION), fun(in_fun) { }

      ValueType & operator=(const ValueType &) = default;
      ValueType & operator=(emp::Datum in_val) { type = VALUE; value = in_val; return *this; }
      ValueType & operator=(double in_val) { type = VALUE; value = in_val; return *this; }
      ValueType & operator=(const std::string & in_val) { type = VALUE; value = in_val; return *this; }
      ValueType & operator=(value_fun_t in_fun) { type = FUNCTION; fun = in_fun; return *this; }

      value_fun_t AsFun() {
        if (type==FUNCTION) return fun; else return [v=value](const emp::DataMap &){ return v; };
      }
    };

    struct BinaryOperator {
      using fun_t = std::function<emp::Datum(emp::Datum,emp::Datum)>;
      size_t prec;
      fun_t fun;
      void Set(size_t in_prec, fun_t in_fun) { prec = in_prec; fun = in_fun; }
    };

    struct Function {
      using fun0_t = std::function<emp::Datum()>;
      using fun1_t = std::function<emp::Datum(emp::Datum)>;
      using fun2_t = std::function<emp::Datum(emp::Datum,emp::Datum)>;
      using fun3_t = std::function<emp::Datum(emp::Datum,emp::Datum,emp::Datum)>;

      size_t num_args = 0;
      fun0_t fun0; fun1_t fun1; fun2_t fun2; fun3_t fun3;

      void Set0(fun0_t in_fun) { num_args = 0; fun0 = in_fun; }
      void Set1(fun1_t in_fun) { num_args = 1; fun1 = in_fun; }
      void Set2(fun2_t in_fun) { num_args = 2; fun2 = in_fun; }
      void Set3(fun3_t in_fun) { num_args = 3; fun3 = in_fun; }
    };

    // --------- MEMBER VARIABLES -----------
    DataMapLexer lexer;

    // Operators and functions that should be used when parsing.
    std::unordered_map<std::string, std::function<emp::Datum(emp::Datum)>> unary_ops;
    std::unordered_map<std::string, BinaryOperator> binary_ops;
    std::unordered_map<std::string, Function> functions;
    emp::vector<emp::Datum> external_vals;

    // The set of data map entries accessed when the last function was parsed.
    std::set<std::string> dm_names;

    // Track the number of errors and the function to call when errors occur.
    template<typename... Ts>
    ValueType ParseError(Ts &&... args) {
      emp::notify::Exception("DataMapParser::PARSE_ERROR", emp::to_string(args...), this);
      return ValueType{};
    }

  public:
    DataMapParser(bool use_defaults=true) {
      if (use_defaults) {
        AddDefaultOperators();
        AddDefaultFunctions();
      }
    }

    DataMapParser(bool use_defaults, emp::Random & random) : DataMapParser(use_defaults)
    { AddRandomFunctions(random); }

    /// Get the set of names that the most recently generated function accesses in DataMap.
    const std::set<std::string> & GetNamesUsed() const { return dm_names; }

    /// Get the set of names used in the provided equation.
    const std::set<std::string> & GetNamesUsed(const std::string & expression) {
      dm_names.clear();
      emp::TokenStream tokens = lexer.Tokenize(expression, std::string("Expression: ") + expression);
      for (emp::Token token : tokens) {
        if (lexer.IsID(token) && !emp::Has(functions, token.lexeme)) {
          dm_names.insert(token.lexeme);
        }
      }
      return dm_names;
    }


    /// Add a unary operator
    void AddOp(const std::string & op, std::function<emp::Datum(emp::Datum)> fun) {
      unary_ops[op] = fun;
    }

    static int ApproxCompare(double x, double y) {
      static constexpr double APPROX_FRACTION = 8192.0;
      double margin = y / APPROX_FRACTION;
      if (x < y - margin) return -1;
      if (x > y + margin) return 1;
      return 0;
    }

    void AddDefaultOperators() {
      // Setup the unary operators for the parser.
      AddOp("+", [](emp::Datum x) { return x; });
      AddOp("-", [](emp::Datum x) { return -x; });
      AddOp("!", [](emp::Datum x) { return !x; });


      // Setup the default binary operators for the parser.
      size_t prec = 0;  // Precedence level of each operator...
      binary_ops["||"].Set( ++prec, [](emp::Datum x, emp::Datum y){ return (x!=0.0)||(y!=0.0); } );
      binary_ops["&&"].Set( ++prec, [](emp::Datum x, emp::Datum y){ return (x!=0.0)&&(y!=0.0); } );
      binary_ops["=="].Set( ++prec, [](emp::Datum x, emp::Datum y){ return x == y; } );
      binary_ops["!="].Set(   prec, [](emp::Datum x, emp::Datum y){ return x != y; } );
      binary_ops["~=="].Set(  prec, [](emp::Datum x, emp::Datum y){ return ApproxCompare(x,y) == 0; } );
      binary_ops["~!="].Set(  prec, [](emp::Datum x, emp::Datum y){ return ApproxCompare(x,y) != 0; } );
      binary_ops["<"] .Set( ++prec, [](emp::Datum x, emp::Datum y){ return x < y; } );
      binary_ops["<="].Set(   prec, [](emp::Datum x, emp::Datum y){ return x <= y; } );
      binary_ops[">"] .Set(   prec, [](emp::Datum x, emp::Datum y){ return x > y; } );
      binary_ops[">="].Set(   prec, [](emp::Datum x, emp::Datum y){ return x >= y; } );
      binary_ops["~<"].Set(   prec, [](emp::Datum x, emp::Datum y){ return ApproxCompare(x,y) == -1; } );
      binary_ops["~<="].Set(  prec, [](emp::Datum x, emp::Datum y){ return ApproxCompare(x,y) != 1; } );
      binary_ops["~>"].Set(   prec, [](emp::Datum x, emp::Datum y){ return ApproxCompare(x,y) == 1; } );
      binary_ops["~>="].Set(  prec, [](emp::Datum x, emp::Datum y){ return ApproxCompare(x,y) != -1; } );
      binary_ops["+"] .Set( ++prec, [](emp::Datum x, emp::Datum y){ return x + y; } );
      binary_ops["-"] .Set(   prec, [](emp::Datum x, emp::Datum y){ return x - y; } );
      binary_ops["*"] .Set( ++prec, [](emp::Datum x, emp::Datum y){ return x * y; } );
      binary_ops["/"] .Set(   prec, [](emp::Datum x, emp::Datum y){ return x / y; } );
      binary_ops["%"] .Set(   prec, [](emp::Datum x, emp::Datum y){ return emp::Mod(x, y); } );
      binary_ops["**"].Set( ++prec, [](emp::Datum x, emp::Datum y){ return emp::Pow(x, y); } );
      binary_ops["%%"].Set(   prec, [](emp::Datum x, emp::Datum y){ return emp::Log(x, y); } );
    }

    void AddDefaultFunctions() {
      // Setup the default functions.
      functions["ABS"].Set1( [](emp::Datum x){ return std::abs(x); } );
      functions["EXP"].Set1( [](emp::Datum x){ return emp::Pow(emp::E, x); } );
      functions["LOG"].Set1( [](emp::Datum x){ return std::log(x); } );
      functions["LOG2"].Set1( [](emp::Datum x){ return std::log2(x); } );
      functions["LOG10"].Set1( [](emp::Datum x){ return std::log10(x); } );

      functions["SQRT"].Set1( [](emp::Datum x){ return std::sqrt(x); } );
      functions["CBRT"].Set1( [](emp::Datum x){ return std::cbrt(x); } );

      functions["SIN"].Set1( [](emp::Datum x){ return std::sin(x); } );
      functions["COS"].Set1( [](emp::Datum x){ return std::cos(x); } );
      functions["TAN"].Set1( [](emp::Datum x){ return std::tan(x); } );
      functions["ASIN"].Set1( [](emp::Datum x){ return std::asin(x); } );
      functions["ACOS"].Set1( [](emp::Datum x){ return std::acos(x); } );
      functions["ATAN"].Set1( [](emp::Datum x){ return std::atan(x); } );
      functions["SINH"].Set1( [](emp::Datum x){ return std::sinh(x); } );
      functions["COSH"].Set1( [](emp::Datum x){ return std::cosh(x); } );
      functions["TANH"].Set1( [](emp::Datum x){ return std::tanh(x); } );
      functions["ASINH"].Set1( [](emp::Datum x){ return std::asinh(x); } );
      functions["ACOSH"].Set1( [](emp::Datum x){ return std::acosh(x); } );
      functions["ATANH"].Set1( [](emp::Datum x){ return std::atanh(x); } );

      functions["CEIL"].Set1( [](emp::Datum x){ return std::ceil(x); } );
      functions["FLOOR"].Set1( [](emp::Datum x){ return std::floor(x); } );
      functions["ROUND"].Set1( [](emp::Datum x){ return std::round(x); } );

      functions["ISINF"].Set1( [](emp::Datum x){ return std::isinf(x); } );
      functions["ISNAN"].Set1( [](emp::Datum x){ return std::isnan(x); } );

      // Default 2-input functions
      functions["HYPOT"].Set2( [](emp::Datum x, emp::Datum y){ return std::hypot(x,y); } );
      functions["EXP"].Set2( [](emp::Datum x, emp::Datum y){ return emp::Pow(x,y); } );
      functions["LOG"].Set2( [](emp::Datum x, emp::Datum y){ return emp::Log(x,y); } );
      functions["MIN"].Set2( [](emp::Datum x, emp::Datum y){ return (x<y) ? x : y; } );
      functions["MAX"].Set2( [](emp::Datum x, emp::Datum y){ return (x>y) ? x : y; } );
      functions["POW"].Set2( [](emp::Datum x, emp::Datum y){ return emp::Pow(x,y); } );

      // Default 3-input functions.
      functions["IF"].Set3( [](emp::Datum x, emp::Datum y, emp::Datum z){
        return (x!=0.0) ? y : z;
      } );
      functions["CLAMP"].Set3( [](emp::Datum x, emp::Datum y, emp::Datum z){
        return (x<y) ? y : (x>z) ? z : x;
      } );
      functions["TO_SCALE"].Set3( [](emp::Datum x, emp::Datum y, emp::Datum z){
        return (z-y)*x+y;
      } );
      functions["FROM_SCALE"].Set3( [](emp::Datum x, emp::Datum y, emp::Datum z){
        return (x-y) / (z-y);
      } );
    }

    void AddRandomFunctions(Random & random) {
      functions["RAND"].Set0( [&random](){ return random.GetDouble(); } );
      functions["RAND"].Set1( [&random](emp::Datum x){ return random.GetDouble(x); } );
      functions["RAND"].Set2( [&random](emp::Datum x, emp::Datum y){ return random.GetDouble(x,y); } );
    }

    /// Helpers for parsing.
    ValueType ParseValue(const DataLayout & layout, pos_t & pos) {
      if constexpr (verbose) {
        std::cout << "ParseValue at position " << pos.GetIndex() << " : " << pos->lexeme << std::endl;
      }

      // Deal with any unary operators...
      if (emp::Has(unary_ops, pos->lexeme)) {
        if constexpr (verbose) std::cout << "Found UNARY OP: " << pos->lexeme << std::endl;
        auto op = unary_ops[pos->lexeme];
        ++pos;
        ValueType val = ParseValue(layout, pos);
        if (val.type == ValueType::VALUE) { return op(val.value); }
        else { return (value_fun_t) [fun=val.fun,op](const emp::DataMap & dm){ return op(fun(dm)); }; }
      }

      // If we have parentheses, process the contents
      if (pos->lexeme == "(") {
        if constexpr (verbose) std::cout << "Found: OPEN PAREN" << std::endl;
        ++pos;
        ValueType val = ParseMath(layout, pos);
        if (pos->lexeme != ")") return ParseError("Expected ')', but found '", pos->lexeme, "'.");
        ++pos;
        return val;
      }

      // If this is a value, set it and return.
      if (lexer.IsNumber(*pos)) {
        double result = emp::from_string<double>(pos->lexeme);
        ++pos;
        return result;
      }

      // Similar for an external value
      if (lexer.IsExternal(*pos)) {
        size_t id = emp::from_string<size_t>(pos->lexeme.substr(1));
        ++pos;
        if (id >= external_vals.size()) {
          ParseError("Invalid access into external variable (\"$", id, "\"): Does not exist.");
        }
        return external_vals[id];
      }

      // Otherwise it should be and identifier!
      const std::string & name = pos->lexeme;
      ++pos;

      // If it is followed by a parenthesis, it should be a function.
      const bool is_fun = (pos.IsValid() && pos->lexeme == "(");

      if (is_fun) {
        if (!emp::Has(functions, name)) return ParseError("Call to unknown function '", name,"'.");
        ++pos;
        emp::vector<ValueType> args;
        while(pos->lexeme != ")") {
          args.push_back(ParseMath(layout, pos));
          if (pos->lexeme == ",") ++pos;
        }
        ++pos;

        // Now build the function based on its argument count.
        value_fun_t out_fun;
        switch (args.size()) {
        case 0:
          if (!functions[name].fun0) ParseError("Function '", name, "' requires arguments.");
          out_fun = [fun=functions[name].fun0](const emp::DataMap & /*dm*/) { return fun(); };
          break;
        case 1:
          if (!functions[name].fun1) ParseError("Function '", name, "' cannot have 1 arguments.");
          out_fun = [fun=functions[name].fun1,arg0=args[0].AsFun()](const emp::DataMap & dm) {
            return fun(arg0(dm));
          };
          break;
        case 2:
          if (!functions[name].fun2) ParseError("Function '", name, "' cannot have 2 arguments.");
          out_fun = [fun=functions[name].fun2,arg0=args[0].AsFun(),arg1=args[1].AsFun()](const emp::DataMap & dm) {
            return fun(arg0(dm), arg1(dm));
          };
          break;
        case 3:
          if (!functions[name].fun3) ParseError("Function '", name, "' cannot have 3 arguments.");
          out_fun = [fun=functions[name].fun3,arg0=args[0].AsFun(),arg1=args[1].AsFun(),arg2=args[2].AsFun()](const emp::DataMap & dm) {
            return fun(arg0(dm), arg1(dm), arg2(dm));
          };
          break;
        default:
          ParseError("Too many arguments for function '", name, "'.");
        }
        return out_fun;
      }

      // This must be a DataLayout entry name.
      if (!layout.HasName(name)) ParseError("Unknown data map entry '", name, "'.");
      dm_names.insert(name);    // Store this name in the list of those used.
      return emp::DataMap::MakeDatumAccessor(layout, name);
    }

    ValueType ParseMath(const DataLayout & layout, pos_t & pos, size_t prec_limit=0) {
      ValueType val1 = ParseValue(layout, pos);

      if constexpr (verbose) {
        if (pos.IsValid()) {
          std::cout << "ParseMath at " << pos.GetIndex() << " : " << pos->lexeme << std::endl;
        } else std::cout << "PROCESSED!" << std::endl;
      }

      while (pos.IsValid() && pos->lexeme != ")" && pos->lexeme != ",") {
        if constexpr (verbose) { std::cout << "...Scanning for op... [" << pos->lexeme << "]" << std::endl;   }

        // If we have an operator, act on it!
        if (Has(binary_ops, pos->lexeme)) {
          const BinaryOperator & op = binary_ops[pos->lexeme];
          if (prec_limit >= op.prec) return val1; // Precedence not allowed; return currnet value.
          ++pos;
          ValueType val2 = ParseMath(layout, pos, op.prec);
          if (val1.type == ValueType::VALUE) {
            if (val2.type == ValueType::VALUE) { val1 = op.fun(val1.value, val2.value); }
            else {
              val1 = (value_fun_t) [val1_num=val1.value,val2_fun=val2.fun,op_fun=op.fun](const emp::DataMap & dm){
                return op_fun(val1_num, val2_fun(dm));
              };
            }
          } else {
            if (val2.type == ValueType::VALUE) {
              val1 = (value_fun_t) [val1_fun=val1.fun,val2_num=val2.value,op_fun=op.fun](const emp::DataMap & dm){
                return op_fun(val1_fun(dm), val2_num);
              };
            } else {
              val1 = (value_fun_t) [val1_fun=val1.fun,val2_fun=val2.fun,op_fun=op.fun](const emp::DataMap & dm){
                return op_fun(val1_fun(dm), val2_fun(dm));
              };
            }
          }
        }

        else ParseError("Operator '", pos->lexeme, "' NOT found!");
      }

      // @CAO Make sure there's not a illegal lexeme here.

      return val1;
    }

    /// Parse a function description that will take a DataMap and return the results.
    /// For example, if the string "foo * 2 + bar" is passed in, a function will be returned
    /// that takes a DataMap (of the example type) loads in the values of "foo" and "bar", and
    /// returns the result of the above equation.

    template <typename... EXTRA_Ts>
    value_fun_t BuildMathFunction(
      const DataLayout & layout,        ///< The layout to use, indicating positions of traits.
      const std::string & expression    ///< The primary expression to convert.
    ) {
      emp::TokenStream tokens = lexer.Tokenize(expression, std::string("Expression: ") + expression);
      if constexpr (verbose) tokens.Print();
      dm_names.clear();    // Reset the names used from data map.
      pos_t pos = tokens.begin();
      ValueType val = ParseMath(layout, pos);

      // If this value is fixed, turn it into a function.
      if (val.type == ValueType::VALUE) {
        return [out=val.value](const emp::DataMap &){ return out; };
      }

      // Otherwise return the function produced.
      #ifdef NDEBUG
        return val.fun;
      #else
        // If we are in debug mode, save the original DataMap and double-check compatability.
        return [fun=val.fun,&orig_layout=layout](const emp::DataMap & dm){
          emp_assert(dm.HasLayout(orig_layout));
          return fun(dm);
      };
      #endif
    }

    /// BuildMathFunction can have extra, external values provided, to be accesses as $0, $1, etc.
    /// These should be either a set of individual values that can each be stored in emp::Datum
    /// OR a single vector of emp::Datum.
    template <typename EXTRA1_T, typename... EXTRA_Ts>
    value_fun_t BuildMathFunction(
      const DataLayout & layout,        ///< The layout to use, indicating positions of traits.
      const std::string & expression,   ///< The primary expression to convert.
      const EXTRA1_T & extra1,          ///< Extra value argument, accessed as $0
      EXTRA_Ts... extras                ///< Extra value arguments (accessed as $1, $2, etc.)
    ) {
      // If we have a vector, make sure it is valid and then just pass it along.
      if constexpr (emp::is_emp_vector<EXTRA1_T>()) {
        static_assert(!emp::is_emp_vector<EXTRA1_T>() || sizeof...(EXTRA_Ts) == 0,
          "If an emp::vector is provided to BuildMathFunction, cannot have additional args.");
        using value_t = typename EXTRA1_T::value_type;
        static_assert(std::is_same<value_t, emp::Datum>(),
          "If BuildMathFunction is provided a vector, it must contain only emp::Datum.");
        external_vals = extra1;
      }
      // Otherwise convert all args to emp::Datum.
      else {
        external_vals = emp::vector<emp::Datum>{extra1, static_cast<emp::Datum>(extras)...};
      }
      return BuildMathFunction(layout, expression);
    }


    /// Convert a DataMap into its layout if needed before generating a lambda based on a
    /// provided expression.
    /// @param dm The DataMap containing the required variables.
    /// @param expression The mathematical expression to be run on the data map.
    /// @param extras Any extra values to fill in a $0, $1, etc.
    template <typename... ARG_Ts>
    value_fun_t BuildMathFunction(const DataMap & dm, ARG_Ts &&... args) {
      return BuildMathFunction(dm.GetLayout(), std::forward<ARG_Ts>(args)...);
    }

    /// Generate a temporary math function and immediately run it on the provided arguments.
    /// @param dm The DataMap containing the required variables.
    /// @param expression The mathematical expression to be run on the data map.
    /// @param extras Any extra values to fill in a $0, $1, etc.
    template <typename... ARG_Ts>
    emp::Datum RunMathFunction(const DataMap & dm, ARG_Ts... args) {
      auto fun = BuildMathFunction(dm.GetLayout(), std::forward<ARG_Ts>(args)...);
      return fun(dm);
    }

  };
}

#endif // #ifndef EMP_DATA_DATAMAPPARSER_HPP_INCLUDE
