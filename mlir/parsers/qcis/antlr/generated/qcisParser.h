
// Generated from qcis.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"


namespace qcis {


class  qcisParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, SEMICOLON = 6, LPAREN = 7, 
    RPAREN = 8, LBRACKET = 9, RBRACKET = 10, COLON = 11, COMMA = 12, DOT = 13, 
    EQUALS = 14, PLUS = 15, MINUS = 16, STAR = 17, SLASH = 18, Integer = 19, 
    QubitId = 20, Constant = 21, RealNumber = 22, StringLiteral = 23, Identifier = 24, 
    Whitespace = 25, LineComment = 26
  };

  enum {
    RuleProgram = 0, RuleQuantumCircuitDeclaration = 1, RuleCircuitName = 2, 
    RuleQuantumStatement = 3, RuleMeasurementStatement = 4, RuleQuantumGateCall = 5, 
    RuleQuantumBarrier = 6, RuleQuantumGateName = 7, RuleQuantumMeasurement = 8, 
    RuleGateArgumentList = 9, RuleMeasurementArgumentList = 10, RuleGateArgument = 11, 
    RuleExpression = 12, RuleExpressionTerminator = 13, RuleListExpression = 14, 
    RuleMeasurementArgument = 15, RuleQuantumCount = 16, RuleClassicalCount = 17, 
    RuleQubitReference = 18, RuleGateParameter = 19, RuleClassicalReference = 20
  };

  explicit qcisParser(antlr4::TokenStream *input);
  ~qcisParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  class ProgramContext;
  class QuantumCircuitDeclarationContext;
  class CircuitNameContext;
  class QuantumStatementContext;
  class MeasurementStatementContext;
  class QuantumGateCallContext;
  class QuantumBarrierContext;
  class QuantumGateNameContext;
  class QuantumMeasurementContext;
  class GateArgumentListContext;
  class MeasurementArgumentListContext;
  class GateArgumentContext;
  class ExpressionContext;
  class ExpressionTerminatorContext;
  class ListExpressionContext;
  class MeasurementArgumentContext;
  class QuantumCountContext;
  class ClassicalCountContext;
  class QubitReferenceContext;
  class GateParameterContext;
  class ClassicalReferenceContext; 

  class  ProgramContext : public antlr4::ParserRuleContext {
  public:
    ProgramContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QuantumCircuitDeclarationContext *quantumCircuitDeclaration();
    std::vector<QuantumStatementContext *> quantumStatement();
    QuantumStatementContext* quantumStatement(size_t i);
    std::vector<MeasurementStatementContext *> measurementStatement();
    MeasurementStatementContext* measurementStatement(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ProgramContext* program();

  class  QuantumCircuitDeclarationContext : public antlr4::ParserRuleContext {
  public:
    QuantumCircuitDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    CircuitNameContext *circuitName();
    antlr4::tree::TerminalNode *EQUALS();
    antlr4::tree::TerminalNode *LPAREN();
    QuantumCountContext *quantumCount();
    antlr4::tree::TerminalNode *RPAREN();
    antlr4::tree::TerminalNode *SEMICOLON();
    antlr4::tree::TerminalNode *COMMA();
    ClassicalCountContext *classicalCount();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QuantumCircuitDeclarationContext* quantumCircuitDeclaration();

  class  CircuitNameContext : public antlr4::ParserRuleContext {
  public:
    CircuitNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Identifier();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CircuitNameContext* circuitName();

  class  QuantumStatementContext : public antlr4::ParserRuleContext {
  public:
    QuantumStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QuantumGateCallContext *quantumGateCall();
    QuantumBarrierContext *quantumBarrier();
    antlr4::tree::TerminalNode *SEMICOLON();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QuantumStatementContext* quantumStatement();

  class  MeasurementStatementContext : public antlr4::ParserRuleContext {
  public:
    MeasurementStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QuantumMeasurementContext *quantumMeasurement();
    antlr4::tree::TerminalNode *SEMICOLON();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MeasurementStatementContext* measurementStatement();

  class  QuantumGateCallContext : public antlr4::ParserRuleContext {
  public:
    QuantumGateCallContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QuantumGateNameContext *quantumGateName();
    std::vector<antlr4::tree::TerminalNode *> QubitId();
    antlr4::tree::TerminalNode* QubitId(size_t i);
    GateArgumentListContext *gateArgumentList();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QuantumGateCallContext* quantumGateCall();

  class  QuantumBarrierContext : public antlr4::ParserRuleContext {
  public:
    QuantumBarrierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> QubitId();
    antlr4::tree::TerminalNode* QubitId(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QuantumBarrierContext* quantumBarrier();

  class  QuantumGateNameContext : public antlr4::ParserRuleContext {
  public:
    QuantumGateNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Identifier();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QuantumGateNameContext* quantumGateName();

  class  QuantumMeasurementContext : public antlr4::ParserRuleContext {
  public:
    QuantumMeasurementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MeasurementArgumentListContext *measurementArgumentList();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QuantumMeasurementContext* quantumMeasurement();

  class  GateArgumentListContext : public antlr4::ParserRuleContext {
  public:
    GateArgumentListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  GateArgumentListContext* gateArgumentList();

  class  MeasurementArgumentListContext : public antlr4::ParserRuleContext {
  public:
    MeasurementArgumentListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *QubitId();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MeasurementArgumentListContext* measurementArgumentList();

  class  GateArgumentContext : public antlr4::ParserRuleContext {
  public:
    GateArgumentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    GateArgumentContext() = default;
    void copyFrom(GateArgumentContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpressionArgumentContext : public GateArgumentContext {
  public:
    ExpressionArgumentContext(GateArgumentContext *ctx);

    ExpressionContext *expression();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IntegerGateArgumentContext : public GateArgumentContext {
  public:
    IntegerGateArgumentContext(GateArgumentContext *ctx);

    antlr4::tree::TerminalNode *Integer();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  GateArgumentContext* gateArgument();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionTerminatorContext *expressionTerminator();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionContext* expression();

  class  ExpressionTerminatorContext : public antlr4::ParserRuleContext {
  public:
    ExpressionTerminatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Constant();
    antlr4::tree::TerminalNode *Integer();
    antlr4::tree::TerminalNode *RealNumber();
    antlr4::tree::TerminalNode *Identifier();
    antlr4::tree::TerminalNode *StringLiteral();
    ListExpressionContext *listExpression();
    antlr4::tree::TerminalNode *MINUS();
    ExpressionTerminatorContext *expressionTerminator();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionTerminatorContext* expressionTerminator();

  class  ListExpressionContext : public antlr4::ParserRuleContext {
  public:
    ListExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LBRACKET();
    std::vector<antlr4::tree::TerminalNode *> Integer();
    antlr4::tree::TerminalNode* Integer(size_t i);
    antlr4::tree::TerminalNode *RBRACKET();
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ListExpressionContext* listExpression();

  class  MeasurementArgumentContext : public antlr4::ParserRuleContext {
  public:
    MeasurementArgumentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    MeasurementArgumentContext() = default;
    void copyFrom(MeasurementArgumentContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  QubitMeasurementArgumentContext : public MeasurementArgumentContext {
  public:
    QubitMeasurementArgumentContext(MeasurementArgumentContext *ctx);

    QubitReferenceContext *qubitReference();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IntegerMeasurementArgumentContext : public MeasurementArgumentContext {
  public:
    IntegerMeasurementArgumentContext(MeasurementArgumentContext *ctx);

    antlr4::tree::TerminalNode *Integer();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  MeasurementArgumentContext* measurementArgument();

  class  QuantumCountContext : public antlr4::ParserRuleContext {
  public:
    QuantumCountContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Integer();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QuantumCountContext* quantumCount();

  class  ClassicalCountContext : public antlr4::ParserRuleContext {
  public:
    ClassicalCountContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Integer();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassicalCountContext* classicalCount();

  class  QubitReferenceContext : public antlr4::ParserRuleContext {
  public:
    QubitReferenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LBRACKET();
    antlr4::tree::TerminalNode *Integer();
    antlr4::tree::TerminalNode *RBRACKET();
    antlr4::tree::TerminalNode *QubitId();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QubitReferenceContext* qubitReference();

  class  GateParameterContext : public antlr4::ParserRuleContext {
  public:
    GateParameterContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *RealNumber();
    antlr4::tree::TerminalNode *Integer();
    antlr4::tree::TerminalNode *Constant();
    antlr4::tree::TerminalNode *MINUS();
    GateParameterContext *gateParameter();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  GateParameterContext* gateParameter();

  class  ClassicalReferenceContext : public antlr4::ParserRuleContext {
  public:
    ClassicalReferenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LBRACKET();
    antlr4::tree::TerminalNode *Integer();
    antlr4::tree::TerminalNode *RBRACKET();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassicalReferenceContext* classicalReference();


private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

}  // namespace qcis
