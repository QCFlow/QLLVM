
// Generated from qiskit.g4 by ANTLR 4.9.2


#include "qiskitListener.h"
#include "qiskitVisitor.h"

#include "qiskitParser.h"


using namespace antlrcpp;
using namespace qiskit;
using namespace antlr4;

qiskitParser::qiskitParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

qiskitParser::~qiskitParser() {
  delete _interpreter;
}

std::string qiskitParser::getGrammarFileName() const {
  return "qiskit.g4";
}

const std::vector<std::string>& qiskitParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& qiskitParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- ProgramContext ------------------------------------------------------------------

qiskitParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::QuantumCircuitDeclarationContext* qiskitParser::ProgramContext::quantumCircuitDeclaration() {
  return getRuleContext<qiskitParser::QuantumCircuitDeclarationContext>(0);
}

std::vector<qiskitParser::QuantumStatementContext *> qiskitParser::ProgramContext::quantumStatement() {
  return getRuleContexts<qiskitParser::QuantumStatementContext>();
}

qiskitParser::QuantumStatementContext* qiskitParser::ProgramContext::quantumStatement(size_t i) {
  return getRuleContext<qiskitParser::QuantumStatementContext>(i);
}

std::vector<qiskitParser::MeasurementStatementContext *> qiskitParser::ProgramContext::measurementStatement() {
  return getRuleContexts<qiskitParser::MeasurementStatementContext>();
}

qiskitParser::MeasurementStatementContext* qiskitParser::ProgramContext::measurementStatement(size_t i) {
  return getRuleContext<qiskitParser::MeasurementStatementContext>(i);
}


size_t qiskitParser::ProgramContext::getRuleIndex() const {
  return qiskitParser::RuleProgram;
}

void qiskitParser::ProgramContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProgram(this);
}

void qiskitParser::ProgramContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProgram(this);
}


antlrcpp::Any qiskitParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::ProgramContext* qiskitParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 0, qiskitParser::RuleProgram);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(46);
    quantumCircuitDeclaration();
    setState(51);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == qiskitParser::Identifier) {
      setState(49);
      _errHandler->sync(this);
      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
      case 1: {
        setState(47);
        quantumStatement();
        break;
      }

      case 2: {
        setState(48);
        measurementStatement();
        break;
      }

      default:
        break;
      }
      setState(53);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumCircuitDeclarationContext ------------------------------------------------------------------

qiskitParser::QuantumCircuitDeclarationContext::QuantumCircuitDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::CircuitNameContext* qiskitParser::QuantumCircuitDeclarationContext::circuitName() {
  return getRuleContext<qiskitParser::CircuitNameContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumCircuitDeclarationContext::EQUALS() {
  return getToken(qiskitParser::EQUALS, 0);
}

tree::TerminalNode* qiskitParser::QuantumCircuitDeclarationContext::LPAREN() {
  return getToken(qiskitParser::LPAREN, 0);
}

qiskitParser::QuantumCountContext* qiskitParser::QuantumCircuitDeclarationContext::quantumCount() {
  return getRuleContext<qiskitParser::QuantumCountContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumCircuitDeclarationContext::RPAREN() {
  return getToken(qiskitParser::RPAREN, 0);
}

tree::TerminalNode* qiskitParser::QuantumCircuitDeclarationContext::SEMICOLON() {
  return getToken(qiskitParser::SEMICOLON, 0);
}

tree::TerminalNode* qiskitParser::QuantumCircuitDeclarationContext::COMMA() {
  return getToken(qiskitParser::COMMA, 0);
}

qiskitParser::ClassicalCountContext* qiskitParser::QuantumCircuitDeclarationContext::classicalCount() {
  return getRuleContext<qiskitParser::ClassicalCountContext>(0);
}


size_t qiskitParser::QuantumCircuitDeclarationContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumCircuitDeclaration;
}

void qiskitParser::QuantumCircuitDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumCircuitDeclaration(this);
}

void qiskitParser::QuantumCircuitDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumCircuitDeclaration(this);
}


antlrcpp::Any qiskitParser::QuantumCircuitDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumCircuitDeclaration(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumCircuitDeclarationContext* qiskitParser::quantumCircuitDeclaration() {
  QuantumCircuitDeclarationContext *_localctx = _tracker.createInstance<QuantumCircuitDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 2, qiskitParser::RuleQuantumCircuitDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(54);
    circuitName();
    setState(55);
    match(qiskitParser::EQUALS);
    setState(56);
    match(qiskitParser::T__0);
    setState(57);
    match(qiskitParser::LPAREN);
    setState(58);
    quantumCount();
    setState(61);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == qiskitParser::COMMA) {
      setState(59);
      match(qiskitParser::COMMA);
      setState(60);
      classicalCount();
    }
    setState(63);
    match(qiskitParser::RPAREN);
    setState(64);
    match(qiskitParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CircuitNameContext ------------------------------------------------------------------

qiskitParser::CircuitNameContext::CircuitNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::CircuitNameContext::Identifier() {
  return getToken(qiskitParser::Identifier, 0);
}


size_t qiskitParser::CircuitNameContext::getRuleIndex() const {
  return qiskitParser::RuleCircuitName;
}

void qiskitParser::CircuitNameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCircuitName(this);
}

void qiskitParser::CircuitNameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCircuitName(this);
}


antlrcpp::Any qiskitParser::CircuitNameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitCircuitName(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::CircuitNameContext* qiskitParser::circuitName() {
  CircuitNameContext *_localctx = _tracker.createInstance<CircuitNameContext>(_ctx, getState());
  enterRule(_localctx, 4, qiskitParser::RuleCircuitName);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(66);
    match(qiskitParser::Identifier);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumStatementContext ------------------------------------------------------------------

qiskitParser::QuantumStatementContext::QuantumStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::QuantumGateCallContext* qiskitParser::QuantumStatementContext::quantumGateCall() {
  return getRuleContext<qiskitParser::QuantumGateCallContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumStatementContext::SEMICOLON() {
  return getToken(qiskitParser::SEMICOLON, 0);
}

qiskitParser::QuantumBarrierContext* qiskitParser::QuantumStatementContext::quantumBarrier() {
  return getRuleContext<qiskitParser::QuantumBarrierContext>(0);
}


size_t qiskitParser::QuantumStatementContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumStatement;
}

void qiskitParser::QuantumStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumStatement(this);
}

void qiskitParser::QuantumStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumStatement(this);
}


antlrcpp::Any qiskitParser::QuantumStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumStatement(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumStatementContext* qiskitParser::quantumStatement() {
  QuantumStatementContext *_localctx = _tracker.createInstance<QuantumStatementContext>(_ctx, getState());
  enterRule(_localctx, 6, qiskitParser::RuleQuantumStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(74);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(68);
      quantumGateCall();
      setState(69);
      match(qiskitParser::SEMICOLON);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(71);
      quantumBarrier();
      setState(72);
      match(qiskitParser::SEMICOLON);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MeasurementStatementContext ------------------------------------------------------------------

qiskitParser::MeasurementStatementContext::MeasurementStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::QuantumMeasurementContext* qiskitParser::MeasurementStatementContext::quantumMeasurement() {
  return getRuleContext<qiskitParser::QuantumMeasurementContext>(0);
}

tree::TerminalNode* qiskitParser::MeasurementStatementContext::SEMICOLON() {
  return getToken(qiskitParser::SEMICOLON, 0);
}


size_t qiskitParser::MeasurementStatementContext::getRuleIndex() const {
  return qiskitParser::RuleMeasurementStatement;
}

void qiskitParser::MeasurementStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMeasurementStatement(this);
}

void qiskitParser::MeasurementStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMeasurementStatement(this);
}


antlrcpp::Any qiskitParser::MeasurementStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitMeasurementStatement(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::MeasurementStatementContext* qiskitParser::measurementStatement() {
  MeasurementStatementContext *_localctx = _tracker.createInstance<MeasurementStatementContext>(_ctx, getState());
  enterRule(_localctx, 8, qiskitParser::RuleMeasurementStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(76);
    quantumMeasurement();
    setState(77);
    match(qiskitParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumGateCallContext ------------------------------------------------------------------

qiskitParser::QuantumGateCallContext::QuantumGateCallContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::CircuitNameContext* qiskitParser::QuantumGateCallContext::circuitName() {
  return getRuleContext<qiskitParser::CircuitNameContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumGateCallContext::DOT() {
  return getToken(qiskitParser::DOT, 0);
}

qiskitParser::QuantumGateNameContext* qiskitParser::QuantumGateCallContext::quantumGateName() {
  return getRuleContext<qiskitParser::QuantumGateNameContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumGateCallContext::LPAREN() {
  return getToken(qiskitParser::LPAREN, 0);
}

tree::TerminalNode* qiskitParser::QuantumGateCallContext::RPAREN() {
  return getToken(qiskitParser::RPAREN, 0);
}

tree::TerminalNode* qiskitParser::QuantumGateCallContext::SEMICOLON() {
  return getToken(qiskitParser::SEMICOLON, 0);
}

qiskitParser::GateArgumentListContext* qiskitParser::QuantumGateCallContext::gateArgumentList() {
  return getRuleContext<qiskitParser::GateArgumentListContext>(0);
}


size_t qiskitParser::QuantumGateCallContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumGateCall;
}

void qiskitParser::QuantumGateCallContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumGateCall(this);
}

void qiskitParser::QuantumGateCallContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumGateCall(this);
}


antlrcpp::Any qiskitParser::QuantumGateCallContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumGateCall(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumGateCallContext* qiskitParser::quantumGateCall() {
  QuantumGateCallContext *_localctx = _tracker.createInstance<QuantumGateCallContext>(_ctx, getState());
  enterRule(_localctx, 10, qiskitParser::RuleQuantumGateCall);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(79);
    circuitName();
    setState(80);
    match(qiskitParser::DOT);
    setState(81);
    quantumGateName();
    setState(82);
    match(qiskitParser::LPAREN);
    setState(84);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << qiskitParser::LBRACKET)
      | (1ULL << qiskitParser::MINUS)
      | (1ULL << qiskitParser::Integer)
      | (1ULL << qiskitParser::Constant)
      | (1ULL << qiskitParser::RealNumber)
      | (1ULL << qiskitParser::StringLiteral)
      | (1ULL << qiskitParser::Identifier))) != 0)) {
      setState(83);
      gateArgumentList();
    }
    setState(86);
    match(qiskitParser::RPAREN);
    setState(87);
    match(qiskitParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumGateNameContext ------------------------------------------------------------------

qiskitParser::QuantumGateNameContext::QuantumGateNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::QuantumGateNameContext::Identifier() {
  return getToken(qiskitParser::Identifier, 0);
}


size_t qiskitParser::QuantumGateNameContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumGateName;
}

void qiskitParser::QuantumGateNameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumGateName(this);
}

void qiskitParser::QuantumGateNameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumGateName(this);
}


antlrcpp::Any qiskitParser::QuantumGateNameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumGateName(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumGateNameContext* qiskitParser::quantumGateName() {
  QuantumGateNameContext *_localctx = _tracker.createInstance<QuantumGateNameContext>(_ctx, getState());
  enterRule(_localctx, 12, qiskitParser::RuleQuantumGateName);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(89);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << qiskitParser::T__1)
      | (1ULL << qiskitParser::T__2)
      | (1ULL << qiskitParser::T__3)
      | (1ULL << qiskitParser::T__4)
      | (1ULL << qiskitParser::T__5)
      | (1ULL << qiskitParser::T__6)
      | (1ULL << qiskitParser::T__7)
      | (1ULL << qiskitParser::T__8)
      | (1ULL << qiskitParser::Identifier))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumMeasurementContext ------------------------------------------------------------------

qiskitParser::QuantumMeasurementContext::QuantumMeasurementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::CircuitNameContext* qiskitParser::QuantumMeasurementContext::circuitName() {
  return getRuleContext<qiskitParser::CircuitNameContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumMeasurementContext::DOT() {
  return getToken(qiskitParser::DOT, 0);
}

tree::TerminalNode* qiskitParser::QuantumMeasurementContext::LPAREN() {
  return getToken(qiskitParser::LPAREN, 0);
}

qiskitParser::MeasurementArgumentListContext* qiskitParser::QuantumMeasurementContext::measurementArgumentList() {
  return getRuleContext<qiskitParser::MeasurementArgumentListContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumMeasurementContext::RPAREN() {
  return getToken(qiskitParser::RPAREN, 0);
}

tree::TerminalNode* qiskitParser::QuantumMeasurementContext::SEMICOLON() {
  return getToken(qiskitParser::SEMICOLON, 0);
}


size_t qiskitParser::QuantumMeasurementContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumMeasurement;
}

void qiskitParser::QuantumMeasurementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumMeasurement(this);
}

void qiskitParser::QuantumMeasurementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumMeasurement(this);
}


antlrcpp::Any qiskitParser::QuantumMeasurementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumMeasurement(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumMeasurementContext* qiskitParser::quantumMeasurement() {
  QuantumMeasurementContext *_localctx = _tracker.createInstance<QuantumMeasurementContext>(_ctx, getState());
  enterRule(_localctx, 14, qiskitParser::RuleQuantumMeasurement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(91);
    circuitName();
    setState(92);
    match(qiskitParser::DOT);
    setState(93);
    match(qiskitParser::T__9);
    setState(94);
    match(qiskitParser::LPAREN);
    setState(95);
    measurementArgumentList();
    setState(96);
    match(qiskitParser::RPAREN);
    setState(97);
    match(qiskitParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumBarrierContext ------------------------------------------------------------------

qiskitParser::QuantumBarrierContext::QuantumBarrierContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::CircuitNameContext* qiskitParser::QuantumBarrierContext::circuitName() {
  return getRuleContext<qiskitParser::CircuitNameContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumBarrierContext::DOT() {
  return getToken(qiskitParser::DOT, 0);
}

tree::TerminalNode* qiskitParser::QuantumBarrierContext::LPAREN() {
  return getToken(qiskitParser::LPAREN, 0);
}

tree::TerminalNode* qiskitParser::QuantumBarrierContext::RPAREN() {
  return getToken(qiskitParser::RPAREN, 0);
}

tree::TerminalNode* qiskitParser::QuantumBarrierContext::SEMICOLON() {
  return getToken(qiskitParser::SEMICOLON, 0);
}

qiskitParser::GateArgumentListContext* qiskitParser::QuantumBarrierContext::gateArgumentList() {
  return getRuleContext<qiskitParser::GateArgumentListContext>(0);
}


size_t qiskitParser::QuantumBarrierContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumBarrier;
}

void qiskitParser::QuantumBarrierContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumBarrier(this);
}

void qiskitParser::QuantumBarrierContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumBarrier(this);
}


antlrcpp::Any qiskitParser::QuantumBarrierContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumBarrier(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumBarrierContext* qiskitParser::quantumBarrier() {
  QuantumBarrierContext *_localctx = _tracker.createInstance<QuantumBarrierContext>(_ctx, getState());
  enterRule(_localctx, 16, qiskitParser::RuleQuantumBarrier);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(99);
    circuitName();
    setState(100);
    match(qiskitParser::DOT);
    setState(101);
    match(qiskitParser::T__10);
    setState(102);
    match(qiskitParser::LPAREN);
    setState(104);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << qiskitParser::LBRACKET)
      | (1ULL << qiskitParser::MINUS)
      | (1ULL << qiskitParser::Integer)
      | (1ULL << qiskitParser::Constant)
      | (1ULL << qiskitParser::RealNumber)
      | (1ULL << qiskitParser::StringLiteral)
      | (1ULL << qiskitParser::Identifier))) != 0)) {
      setState(103);
      gateArgumentList();
    }
    setState(106);
    match(qiskitParser::RPAREN);
    setState(107);
    match(qiskitParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GateArgumentListContext ------------------------------------------------------------------

qiskitParser::GateArgumentListContext::GateArgumentListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<qiskitParser::ExpressionContext *> qiskitParser::GateArgumentListContext::expression() {
  return getRuleContexts<qiskitParser::ExpressionContext>();
}

qiskitParser::ExpressionContext* qiskitParser::GateArgumentListContext::expression(size_t i) {
  return getRuleContext<qiskitParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> qiskitParser::GateArgumentListContext::COMMA() {
  return getTokens(qiskitParser::COMMA);
}

tree::TerminalNode* qiskitParser::GateArgumentListContext::COMMA(size_t i) {
  return getToken(qiskitParser::COMMA, i);
}


size_t qiskitParser::GateArgumentListContext::getRuleIndex() const {
  return qiskitParser::RuleGateArgumentList;
}

void qiskitParser::GateArgumentListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterGateArgumentList(this);
}

void qiskitParser::GateArgumentListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitGateArgumentList(this);
}


antlrcpp::Any qiskitParser::GateArgumentListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitGateArgumentList(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::GateArgumentListContext* qiskitParser::gateArgumentList() {
  GateArgumentListContext *_localctx = _tracker.createInstance<GateArgumentListContext>(_ctx, getState());
  enterRule(_localctx, 18, qiskitParser::RuleGateArgumentList);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(114);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(109);
        expression();
        setState(110);
        match(qiskitParser::COMMA); 
      }
      setState(116);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx);
    }
    setState(117);
    expression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MeasurementArgumentListContext ------------------------------------------------------------------

qiskitParser::MeasurementArgumentListContext::MeasurementArgumentListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<qiskitParser::MeasurementArgumentContext *> qiskitParser::MeasurementArgumentListContext::measurementArgument() {
  return getRuleContexts<qiskitParser::MeasurementArgumentContext>();
}

qiskitParser::MeasurementArgumentContext* qiskitParser::MeasurementArgumentListContext::measurementArgument(size_t i) {
  return getRuleContext<qiskitParser::MeasurementArgumentContext>(i);
}

std::vector<tree::TerminalNode *> qiskitParser::MeasurementArgumentListContext::COMMA() {
  return getTokens(qiskitParser::COMMA);
}

tree::TerminalNode* qiskitParser::MeasurementArgumentListContext::COMMA(size_t i) {
  return getToken(qiskitParser::COMMA, i);
}


size_t qiskitParser::MeasurementArgumentListContext::getRuleIndex() const {
  return qiskitParser::RuleMeasurementArgumentList;
}

void qiskitParser::MeasurementArgumentListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMeasurementArgumentList(this);
}

void qiskitParser::MeasurementArgumentListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMeasurementArgumentList(this);
}


antlrcpp::Any qiskitParser::MeasurementArgumentListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitMeasurementArgumentList(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::MeasurementArgumentListContext* qiskitParser::measurementArgumentList() {
  MeasurementArgumentListContext *_localctx = _tracker.createInstance<MeasurementArgumentListContext>(_ctx, getState());
  enterRule(_localctx, 20, qiskitParser::RuleMeasurementArgumentList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(119);
    measurementArgument();
    setState(124);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == qiskitParser::COMMA) {
      setState(120);
      match(qiskitParser::COMMA);
      setState(121);
      measurementArgument();
      setState(126);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

qiskitParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::ExpressionTerminatorContext* qiskitParser::ExpressionContext::expressionTerminator() {
  return getRuleContext<qiskitParser::ExpressionTerminatorContext>(0);
}


size_t qiskitParser::ExpressionContext::getRuleIndex() const {
  return qiskitParser::RuleExpression;
}

void qiskitParser::ExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpression(this);
}

void qiskitParser::ExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpression(this);
}


antlrcpp::Any qiskitParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::ExpressionContext* qiskitParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 22, qiskitParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(127);
    expressionTerminator();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionTerminatorContext ------------------------------------------------------------------

qiskitParser::ExpressionTerminatorContext::ExpressionTerminatorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::ExpressionTerminatorContext::Constant() {
  return getToken(qiskitParser::Constant, 0);
}

tree::TerminalNode* qiskitParser::ExpressionTerminatorContext::Integer() {
  return getToken(qiskitParser::Integer, 0);
}

tree::TerminalNode* qiskitParser::ExpressionTerminatorContext::RealNumber() {
  return getToken(qiskitParser::RealNumber, 0);
}

tree::TerminalNode* qiskitParser::ExpressionTerminatorContext::Identifier() {
  return getToken(qiskitParser::Identifier, 0);
}

tree::TerminalNode* qiskitParser::ExpressionTerminatorContext::StringLiteral() {
  return getToken(qiskitParser::StringLiteral, 0);
}

qiskitParser::ListExpressionContext* qiskitParser::ExpressionTerminatorContext::listExpression() {
  return getRuleContext<qiskitParser::ListExpressionContext>(0);
}

tree::TerminalNode* qiskitParser::ExpressionTerminatorContext::MINUS() {
  return getToken(qiskitParser::MINUS, 0);
}

qiskitParser::ExpressionTerminatorContext* qiskitParser::ExpressionTerminatorContext::expressionTerminator() {
  return getRuleContext<qiskitParser::ExpressionTerminatorContext>(0);
}


size_t qiskitParser::ExpressionTerminatorContext::getRuleIndex() const {
  return qiskitParser::RuleExpressionTerminator;
}

void qiskitParser::ExpressionTerminatorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpressionTerminator(this);
}

void qiskitParser::ExpressionTerminatorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpressionTerminator(this);
}


antlrcpp::Any qiskitParser::ExpressionTerminatorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitExpressionTerminator(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::ExpressionTerminatorContext* qiskitParser::expressionTerminator() {
  ExpressionTerminatorContext *_localctx = _tracker.createInstance<ExpressionTerminatorContext>(_ctx, getState());
  enterRule(_localctx, 24, qiskitParser::RuleExpressionTerminator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(137);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qiskitParser::Constant: {
        enterOuterAlt(_localctx, 1);
        setState(129);
        match(qiskitParser::Constant);
        break;
      }

      case qiskitParser::Integer: {
        enterOuterAlt(_localctx, 2);
        setState(130);
        match(qiskitParser::Integer);
        break;
      }

      case qiskitParser::RealNumber: {
        enterOuterAlt(_localctx, 3);
        setState(131);
        match(qiskitParser::RealNumber);
        break;
      }

      case qiskitParser::Identifier: {
        enterOuterAlt(_localctx, 4);
        setState(132);
        match(qiskitParser::Identifier);
        break;
      }

      case qiskitParser::StringLiteral: {
        enterOuterAlt(_localctx, 5);
        setState(133);
        match(qiskitParser::StringLiteral);
        break;
      }

      case qiskitParser::LBRACKET: {
        enterOuterAlt(_localctx, 6);
        setState(134);
        listExpression();
        break;
      }

      case qiskitParser::MINUS: {
        enterOuterAlt(_localctx, 7);
        setState(135);
        match(qiskitParser::MINUS);
        setState(136);
        expressionTerminator();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ListExpressionContext ------------------------------------------------------------------

qiskitParser::ListExpressionContext::ListExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::ListExpressionContext::LBRACKET() {
  return getToken(qiskitParser::LBRACKET, 0);
}

std::vector<tree::TerminalNode *> qiskitParser::ListExpressionContext::Integer() {
  return getTokens(qiskitParser::Integer);
}

tree::TerminalNode* qiskitParser::ListExpressionContext::Integer(size_t i) {
  return getToken(qiskitParser::Integer, i);
}

tree::TerminalNode* qiskitParser::ListExpressionContext::RBRACKET() {
  return getToken(qiskitParser::RBRACKET, 0);
}

std::vector<tree::TerminalNode *> qiskitParser::ListExpressionContext::COMMA() {
  return getTokens(qiskitParser::COMMA);
}

tree::TerminalNode* qiskitParser::ListExpressionContext::COMMA(size_t i) {
  return getToken(qiskitParser::COMMA, i);
}


size_t qiskitParser::ListExpressionContext::getRuleIndex() const {
  return qiskitParser::RuleListExpression;
}

void qiskitParser::ListExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterListExpression(this);
}

void qiskitParser::ListExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitListExpression(this);
}


antlrcpp::Any qiskitParser::ListExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitListExpression(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::ListExpressionContext* qiskitParser::listExpression() {
  ListExpressionContext *_localctx = _tracker.createInstance<ListExpressionContext>(_ctx, getState());
  enterRule(_localctx, 26, qiskitParser::RuleListExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(139);
    match(qiskitParser::LBRACKET);
    setState(144);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(140);
        match(qiskitParser::Integer);
        setState(141);
        match(qiskitParser::COMMA); 
      }
      setState(146);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx);
    }
    setState(147);
    match(qiskitParser::Integer);
    setState(148);
    match(qiskitParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MeasurementArgumentContext ------------------------------------------------------------------

qiskitParser::MeasurementArgumentContext::MeasurementArgumentContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t qiskitParser::MeasurementArgumentContext::getRuleIndex() const {
  return qiskitParser::RuleMeasurementArgument;
}

void qiskitParser::MeasurementArgumentContext::copyFrom(MeasurementArgumentContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- QubitMeasurementArgumentContext ------------------------------------------------------------------

qiskitParser::QubitReferenceContext* qiskitParser::QubitMeasurementArgumentContext::qubitReference() {
  return getRuleContext<qiskitParser::QubitReferenceContext>(0);
}

qiskitParser::QubitMeasurementArgumentContext::QubitMeasurementArgumentContext(MeasurementArgumentContext *ctx) { copyFrom(ctx); }

void qiskitParser::QubitMeasurementArgumentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQubitMeasurementArgument(this);
}
void qiskitParser::QubitMeasurementArgumentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQubitMeasurementArgument(this);
}

antlrcpp::Any qiskitParser::QubitMeasurementArgumentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQubitMeasurementArgument(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IntegerMeasurementArgumentContext ------------------------------------------------------------------

tree::TerminalNode* qiskitParser::IntegerMeasurementArgumentContext::Integer() {
  return getToken(qiskitParser::Integer, 0);
}

qiskitParser::IntegerMeasurementArgumentContext::IntegerMeasurementArgumentContext(MeasurementArgumentContext *ctx) { copyFrom(ctx); }

void qiskitParser::IntegerMeasurementArgumentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntegerMeasurementArgument(this);
}
void qiskitParser::IntegerMeasurementArgumentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntegerMeasurementArgument(this);
}

antlrcpp::Any qiskitParser::IntegerMeasurementArgumentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitIntegerMeasurementArgument(this);
  else
    return visitor->visitChildren(this);
}
qiskitParser::MeasurementArgumentContext* qiskitParser::measurementArgument() {
  MeasurementArgumentContext *_localctx = _tracker.createInstance<MeasurementArgumentContext>(_ctx, getState());
  enterRule(_localctx, 28, qiskitParser::RuleMeasurementArgument);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(152);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qiskitParser::Integer: {
        _localctx = dynamic_cast<MeasurementArgumentContext *>(_tracker.createInstance<qiskitParser::IntegerMeasurementArgumentContext>(_localctx));
        enterOuterAlt(_localctx, 1);
        setState(150);
        match(qiskitParser::Integer);
        break;
      }

      case qiskitParser::T__11: {
        _localctx = dynamic_cast<MeasurementArgumentContext *>(_tracker.createInstance<qiskitParser::QubitMeasurementArgumentContext>(_localctx));
        enterOuterAlt(_localctx, 2);
        setState(151);
        qubitReference();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumOperationContext ------------------------------------------------------------------

qiskitParser::QuantumOperationContext::QuantumOperationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::QuantumGateNameContext* qiskitParser::QuantumOperationContext::quantumGateName() {
  return getRuleContext<qiskitParser::QuantumGateNameContext>(0);
}

tree::TerminalNode* qiskitParser::QuantumOperationContext::Identifier() {
  return getToken(qiskitParser::Identifier, 0);
}


size_t qiskitParser::QuantumOperationContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumOperation;
}

void qiskitParser::QuantumOperationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumOperation(this);
}

void qiskitParser::QuantumOperationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumOperation(this);
}


antlrcpp::Any qiskitParser::QuantumOperationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumOperation(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumOperationContext* qiskitParser::quantumOperation() {
  QuantumOperationContext *_localctx = _tracker.createInstance<QuantumOperationContext>(_ctx, getState());
  enterRule(_localctx, 30, qiskitParser::RuleQuantumOperation);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(158);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(154);
      quantumGateName();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(155);
      match(qiskitParser::T__9);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(156);
      match(qiskitParser::T__10);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(157);
      match(qiskitParser::Identifier);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Expression_1Context ------------------------------------------------------------------

qiskitParser::Expression_1Context::Expression_1Context(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::TermContext* qiskitParser::Expression_1Context::term() {
  return getRuleContext<qiskitParser::TermContext>(0);
}

qiskitParser::Expression_1Context* qiskitParser::Expression_1Context::expression_1() {
  return getRuleContext<qiskitParser::Expression_1Context>(0);
}

tree::TerminalNode* qiskitParser::Expression_1Context::PLUS() {
  return getToken(qiskitParser::PLUS, 0);
}

tree::TerminalNode* qiskitParser::Expression_1Context::MINUS() {
  return getToken(qiskitParser::MINUS, 0);
}


size_t qiskitParser::Expression_1Context::getRuleIndex() const {
  return qiskitParser::RuleExpression_1;
}

void qiskitParser::Expression_1Context::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpression_1(this);
}

void qiskitParser::Expression_1Context::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpression_1(this);
}


antlrcpp::Any qiskitParser::Expression_1Context::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitExpression_1(this);
  else
    return visitor->visitChildren(this);
}


qiskitParser::Expression_1Context* qiskitParser::expression_1() {
   return expression_1(0);
}

qiskitParser::Expression_1Context* qiskitParser::expression_1(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  qiskitParser::Expression_1Context *_localctx = _tracker.createInstance<Expression_1Context>(_ctx, parentState);
  qiskitParser::Expression_1Context *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 32;
  enterRecursionRule(_localctx, 32, qiskitParser::RuleExpression_1, precedence);

    

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(161);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(171);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(169);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<Expression_1Context>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExpression_1);
          setState(163);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(164);
          match(qiskitParser::PLUS);
          setState(165);
          term(0);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<Expression_1Context>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExpression_1);
          setState(166);

          if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
          setState(167);
          match(qiskitParser::MINUS);
          setState(168);
          term(0);
          break;
        }

        default:
          break;
        } 
      }
      setState(173);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- TermContext ------------------------------------------------------------------

qiskitParser::TermContext::TermContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qiskitParser::FactorContext* qiskitParser::TermContext::factor() {
  return getRuleContext<qiskitParser::FactorContext>(0);
}

qiskitParser::TermContext* qiskitParser::TermContext::term() {
  return getRuleContext<qiskitParser::TermContext>(0);
}

tree::TerminalNode* qiskitParser::TermContext::STAR() {
  return getToken(qiskitParser::STAR, 0);
}

tree::TerminalNode* qiskitParser::TermContext::SLASH() {
  return getToken(qiskitParser::SLASH, 0);
}


size_t qiskitParser::TermContext::getRuleIndex() const {
  return qiskitParser::RuleTerm;
}

void qiskitParser::TermContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTerm(this);
}

void qiskitParser::TermContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTerm(this);
}


antlrcpp::Any qiskitParser::TermContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitTerm(this);
  else
    return visitor->visitChildren(this);
}


qiskitParser::TermContext* qiskitParser::term() {
   return term(0);
}

qiskitParser::TermContext* qiskitParser::term(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  qiskitParser::TermContext *_localctx = _tracker.createInstance<TermContext>(_ctx, parentState);
  qiskitParser::TermContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 34;
  enterRecursionRule(_localctx, 34, qiskitParser::RuleTerm, precedence);

    

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(175);
    factor();
    _ctx->stop = _input->LT(-1);
    setState(185);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(183);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 14, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<TermContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleTerm);
          setState(177);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(178);
          match(qiskitParser::STAR);
          setState(179);
          factor();
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<TermContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleTerm);
          setState(180);

          if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
          setState(181);
          match(qiskitParser::SLASH);
          setState(182);
          factor();
          break;
        }

        default:
          break;
        } 
      }
      setState(187);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- FactorContext ------------------------------------------------------------------

qiskitParser::FactorContext::FactorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::FactorContext::Integer() {
  return getToken(qiskitParser::Integer, 0);
}

tree::TerminalNode* qiskitParser::FactorContext::LPAREN() {
  return getToken(qiskitParser::LPAREN, 0);
}

qiskitParser::Expression_1Context* qiskitParser::FactorContext::expression_1() {
  return getRuleContext<qiskitParser::Expression_1Context>(0);
}

tree::TerminalNode* qiskitParser::FactorContext::RPAREN() {
  return getToken(qiskitParser::RPAREN, 0);
}

qiskitParser::QubitReferenceContext* qiskitParser::FactorContext::qubitReference() {
  return getRuleContext<qiskitParser::QubitReferenceContext>(0);
}

qiskitParser::ClassicalReferenceContext* qiskitParser::FactorContext::classicalReference() {
  return getRuleContext<qiskitParser::ClassicalReferenceContext>(0);
}


size_t qiskitParser::FactorContext::getRuleIndex() const {
  return qiskitParser::RuleFactor;
}

void qiskitParser::FactorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFactor(this);
}

void qiskitParser::FactorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFactor(this);
}


antlrcpp::Any qiskitParser::FactorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitFactor(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::FactorContext* qiskitParser::factor() {
  FactorContext *_localctx = _tracker.createInstance<FactorContext>(_ctx, getState());
  enterRule(_localctx, 36, qiskitParser::RuleFactor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(195);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qiskitParser::Integer: {
        enterOuterAlt(_localctx, 1);
        setState(188);
        match(qiskitParser::Integer);
        break;
      }

      case qiskitParser::LPAREN: {
        enterOuterAlt(_localctx, 2);
        setState(189);
        match(qiskitParser::LPAREN);
        setState(190);
        expression_1(0);
        setState(191);
        match(qiskitParser::RPAREN);
        break;
      }

      case qiskitParser::T__11: {
        enterOuterAlt(_localctx, 3);
        setState(193);
        qubitReference();
        break;
      }

      case qiskitParser::T__12: {
        enterOuterAlt(_localctx, 4);
        setState(194);
        classicalReference();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumCountContext ------------------------------------------------------------------

qiskitParser::QuantumCountContext::QuantumCountContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::QuantumCountContext::Integer() {
  return getToken(qiskitParser::Integer, 0);
}


size_t qiskitParser::QuantumCountContext::getRuleIndex() const {
  return qiskitParser::RuleQuantumCount;
}

void qiskitParser::QuantumCountContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumCount(this);
}

void qiskitParser::QuantumCountContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumCount(this);
}


antlrcpp::Any qiskitParser::QuantumCountContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQuantumCount(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QuantumCountContext* qiskitParser::quantumCount() {
  QuantumCountContext *_localctx = _tracker.createInstance<QuantumCountContext>(_ctx, getState());
  enterRule(_localctx, 38, qiskitParser::RuleQuantumCount);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(197);
    match(qiskitParser::Integer);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassicalCountContext ------------------------------------------------------------------

qiskitParser::ClassicalCountContext::ClassicalCountContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::ClassicalCountContext::Integer() {
  return getToken(qiskitParser::Integer, 0);
}


size_t qiskitParser::ClassicalCountContext::getRuleIndex() const {
  return qiskitParser::RuleClassicalCount;
}

void qiskitParser::ClassicalCountContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassicalCount(this);
}

void qiskitParser::ClassicalCountContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassicalCount(this);
}


antlrcpp::Any qiskitParser::ClassicalCountContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitClassicalCount(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::ClassicalCountContext* qiskitParser::classicalCount() {
  ClassicalCountContext *_localctx = _tracker.createInstance<ClassicalCountContext>(_ctx, getState());
  enterRule(_localctx, 40, qiskitParser::RuleClassicalCount);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(199);
    match(qiskitParser::Integer);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QubitReferenceContext ------------------------------------------------------------------

qiskitParser::QubitReferenceContext::QubitReferenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::QubitReferenceContext::LBRACKET() {
  return getToken(qiskitParser::LBRACKET, 0);
}

tree::TerminalNode* qiskitParser::QubitReferenceContext::Integer() {
  return getToken(qiskitParser::Integer, 0);
}

tree::TerminalNode* qiskitParser::QubitReferenceContext::RBRACKET() {
  return getToken(qiskitParser::RBRACKET, 0);
}


size_t qiskitParser::QubitReferenceContext::getRuleIndex() const {
  return qiskitParser::RuleQubitReference;
}

void qiskitParser::QubitReferenceContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQubitReference(this);
}

void qiskitParser::QubitReferenceContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQubitReference(this);
}


antlrcpp::Any qiskitParser::QubitReferenceContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitQubitReference(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::QubitReferenceContext* qiskitParser::qubitReference() {
  QubitReferenceContext *_localctx = _tracker.createInstance<QubitReferenceContext>(_ctx, getState());
  enterRule(_localctx, 42, qiskitParser::RuleQubitReference);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(201);
    match(qiskitParser::T__11);
    setState(202);
    match(qiskitParser::LBRACKET);
    setState(203);
    match(qiskitParser::Integer);
    setState(204);
    match(qiskitParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassicalReferenceContext ------------------------------------------------------------------

qiskitParser::ClassicalReferenceContext::ClassicalReferenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qiskitParser::ClassicalReferenceContext::LBRACKET() {
  return getToken(qiskitParser::LBRACKET, 0);
}

tree::TerminalNode* qiskitParser::ClassicalReferenceContext::Integer() {
  return getToken(qiskitParser::Integer, 0);
}

tree::TerminalNode* qiskitParser::ClassicalReferenceContext::RBRACKET() {
  return getToken(qiskitParser::RBRACKET, 0);
}


size_t qiskitParser::ClassicalReferenceContext::getRuleIndex() const {
  return qiskitParser::RuleClassicalReference;
}

void qiskitParser::ClassicalReferenceContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassicalReference(this);
}

void qiskitParser::ClassicalReferenceContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qiskitListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassicalReference(this);
}


antlrcpp::Any qiskitParser::ClassicalReferenceContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qiskitVisitor*>(visitor))
    return parserVisitor->visitClassicalReference(this);
  else
    return visitor->visitChildren(this);
}

qiskitParser::ClassicalReferenceContext* qiskitParser::classicalReference() {
  ClassicalReferenceContext *_localctx = _tracker.createInstance<ClassicalReferenceContext>(_ctx, getState());
  enterRule(_localctx, 44, qiskitParser::RuleClassicalReference);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(206);
    match(qiskitParser::T__12);
    setState(207);
    match(qiskitParser::LBRACKET);
    setState(208);
    match(qiskitParser::Integer);
    setState(209);
    match(qiskitParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool qiskitParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 16: return expression_1Sempred(dynamic_cast<Expression_1Context *>(context), predicateIndex);
    case 17: return termSempred(dynamic_cast<TermContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool qiskitParser::expression_1Sempred(Expression_1Context *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 2);
    case 1: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

bool qiskitParser::termSempred(TermContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 2: return precpred(_ctx, 2);
    case 3: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

// Static vars and initialization.
std::vector<dfa::DFA> qiskitParser::_decisionToDFA;
atn::PredictionContextCache qiskitParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN qiskitParser::_atn;
std::vector<uint16_t> qiskitParser::_serializedATN;

std::vector<std::string> qiskitParser::_ruleNames = {
  "program", "quantumCircuitDeclaration", "circuitName", "quantumStatement", 
  "measurementStatement", "quantumGateCall", "quantumGateName", "quantumMeasurement", 
  "quantumBarrier", "gateArgumentList", "measurementArgumentList", "expression", 
  "expressionTerminator", "listExpression", "measurementArgument", "quantumOperation", 
  "expression_1", "term", "factor", "quantumCount", "classicalCount", "qubitReference", 
  "classicalReference"
};

std::vector<std::string> qiskitParser::_literalNames = {
  "", "'QuantumCircuit'", "'h'", "'x'", "'y'", "'z'", "'cx'", "'CX'", "'U'", 
  "'reset'", "'measure'", "'barrier'", "'q'", "'c'", "';'", "'('", "')'", 
  "'['", "']'", "':'", "','", "'.'", "'='", "'+'", "'-'", "'*'", "'/'"
};

std::vector<std::string> qiskitParser::_symbolicNames = {
  "", "", "", "", "", "", "", "", "", "", "", "", "", "", "SEMICOLON", "LPAREN", 
  "RPAREN", "LBRACKET", "RBRACKET", "COLON", "COMMA", "DOT", "EQUALS", "PLUS", 
  "MINUS", "STAR", "SLASH", "Integer", "Constant", "RealNumber", "StringLiteral", 
  "Identifier", "Whitespace", "LineComment"
};

dfa::Vocabulary qiskitParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> qiskitParser::_tokenNames;

qiskitParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  static const uint16_t serializedATNSegment0[] = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
       0x3, 0x23, 0xd6, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 
       0x7, 0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 
       0x4, 0xb, 0x9, 0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 
       0xe, 0x9, 0xe, 0x4, 0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 
       0x9, 0x11, 0x4, 0x12, 0x9, 0x12, 0x4, 0x13, 0x9, 0x13, 0x4, 0x14, 
       0x9, 0x14, 0x4, 0x15, 0x9, 0x15, 0x4, 0x16, 0x9, 0x16, 0x4, 0x17, 
       0x9, 0x17, 0x4, 0x18, 0x9, 0x18, 0x3, 0x2, 0x3, 0x2, 0x3, 0x2, 0x7, 
       0x2, 0x34, 0xa, 0x2, 0xc, 0x2, 0xe, 0x2, 0x37, 0xb, 0x2, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x5, 
       0x3, 0x40, 0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 
       0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 
       0x5, 0x5, 0x4d, 0xa, 0x5, 0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 0x7, 
       0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x5, 0x7, 0x57, 0xa, 0x7, 
       0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x8, 0x3, 0x8, 0x3, 0x9, 0x3, 
       0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 
       0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x5, 0xa, 0x6b, 
       0xa, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xb, 0x3, 0xb, 0x3, 
       0xb, 0x7, 0xb, 0x73, 0xa, 0xb, 0xc, 0xb, 0xe, 0xb, 0x76, 0xb, 0xb, 
       0x3, 0xb, 0x3, 0xb, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x7, 0xc, 0x7d, 
       0xa, 0xc, 0xc, 0xc, 0xe, 0xc, 0x80, 0xb, 0xc, 0x3, 0xd, 0x3, 0xd, 
       0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 
       0xe, 0x3, 0xe, 0x5, 0xe, 0x8c, 0xa, 0xe, 0x3, 0xf, 0x3, 0xf, 0x3, 
       0xf, 0x7, 0xf, 0x91, 0xa, 0xf, 0xc, 0xf, 0xe, 0xf, 0x94, 0xb, 0xf, 
       0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0x10, 0x3, 0x10, 0x5, 0x10, 0x9b, 
       0xa, 0x10, 0x3, 0x11, 0x3, 0x11, 0x3, 0x11, 0x3, 0x11, 0x5, 0x11, 
       0xa1, 0xa, 0x11, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 
       0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x7, 0x12, 0xac, 
       0xa, 0x12, 0xc, 0x12, 0xe, 0x12, 0xaf, 0xb, 0x12, 0x3, 0x13, 0x3, 
       0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 
       0x13, 0x3, 0x13, 0x7, 0x13, 0xba, 0xa, 0x13, 0xc, 0x13, 0xe, 0x13, 
       0xbd, 0xb, 0x13, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 
       0x14, 0x3, 0x14, 0x3, 0x14, 0x5, 0x14, 0xc6, 0xa, 0x14, 0x3, 0x15, 
       0x3, 0x15, 0x3, 0x16, 0x3, 0x16, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 
       0x3, 0x17, 0x3, 0x17, 0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 
       0x3, 0x18, 0x3, 0x18, 0x2, 0x4, 0x22, 0x24, 0x19, 0x2, 0x4, 0x6, 
       0x8, 0xa, 0xc, 0xe, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 
       0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x2, 0x3, 0x4, 0x2, 
       0x4, 0xb, 0x21, 0x21, 0x2, 0xd8, 0x2, 0x30, 0x3, 0x2, 0x2, 0x2, 0x4, 
       0x38, 0x3, 0x2, 0x2, 0x2, 0x6, 0x44, 0x3, 0x2, 0x2, 0x2, 0x8, 0x4c, 
       0x3, 0x2, 0x2, 0x2, 0xa, 0x4e, 0x3, 0x2, 0x2, 0x2, 0xc, 0x51, 0x3, 
       0x2, 0x2, 0x2, 0xe, 0x5b, 0x3, 0x2, 0x2, 0x2, 0x10, 0x5d, 0x3, 0x2, 
       0x2, 0x2, 0x12, 0x65, 0x3, 0x2, 0x2, 0x2, 0x14, 0x74, 0x3, 0x2, 0x2, 
       0x2, 0x16, 0x79, 0x3, 0x2, 0x2, 0x2, 0x18, 0x81, 0x3, 0x2, 0x2, 0x2, 
       0x1a, 0x8b, 0x3, 0x2, 0x2, 0x2, 0x1c, 0x8d, 0x3, 0x2, 0x2, 0x2, 0x1e, 
       0x9a, 0x3, 0x2, 0x2, 0x2, 0x20, 0xa0, 0x3, 0x2, 0x2, 0x2, 0x22, 0xa2, 
       0x3, 0x2, 0x2, 0x2, 0x24, 0xb0, 0x3, 0x2, 0x2, 0x2, 0x26, 0xc5, 0x3, 
       0x2, 0x2, 0x2, 0x28, 0xc7, 0x3, 0x2, 0x2, 0x2, 0x2a, 0xc9, 0x3, 0x2, 
       0x2, 0x2, 0x2c, 0xcb, 0x3, 0x2, 0x2, 0x2, 0x2e, 0xd0, 0x3, 0x2, 0x2, 
       0x2, 0x30, 0x35, 0x5, 0x4, 0x3, 0x2, 0x31, 0x34, 0x5, 0x8, 0x5, 0x2, 
       0x32, 0x34, 0x5, 0xa, 0x6, 0x2, 0x33, 0x31, 0x3, 0x2, 0x2, 0x2, 0x33, 
       0x32, 0x3, 0x2, 0x2, 0x2, 0x34, 0x37, 0x3, 0x2, 0x2, 0x2, 0x35, 0x33, 
       0x3, 0x2, 0x2, 0x2, 0x35, 0x36, 0x3, 0x2, 0x2, 0x2, 0x36, 0x3, 0x3, 
       0x2, 0x2, 0x2, 0x37, 0x35, 0x3, 0x2, 0x2, 0x2, 0x38, 0x39, 0x5, 0x6, 
       0x4, 0x2, 0x39, 0x3a, 0x7, 0x18, 0x2, 0x2, 0x3a, 0x3b, 0x7, 0x3, 
       0x2, 0x2, 0x3b, 0x3c, 0x7, 0x11, 0x2, 0x2, 0x3c, 0x3f, 0x5, 0x28, 
       0x15, 0x2, 0x3d, 0x3e, 0x7, 0x16, 0x2, 0x2, 0x3e, 0x40, 0x5, 0x2a, 
       0x16, 0x2, 0x3f, 0x3d, 0x3, 0x2, 0x2, 0x2, 0x3f, 0x40, 0x3, 0x2, 
       0x2, 0x2, 0x40, 0x41, 0x3, 0x2, 0x2, 0x2, 0x41, 0x42, 0x7, 0x12, 
       0x2, 0x2, 0x42, 0x43, 0x7, 0x10, 0x2, 0x2, 0x43, 0x5, 0x3, 0x2, 0x2, 
       0x2, 0x44, 0x45, 0x7, 0x21, 0x2, 0x2, 0x45, 0x7, 0x3, 0x2, 0x2, 0x2, 
       0x46, 0x47, 0x5, 0xc, 0x7, 0x2, 0x47, 0x48, 0x7, 0x10, 0x2, 0x2, 
       0x48, 0x4d, 0x3, 0x2, 0x2, 0x2, 0x49, 0x4a, 0x5, 0x12, 0xa, 0x2, 
       0x4a, 0x4b, 0x7, 0x10, 0x2, 0x2, 0x4b, 0x4d, 0x3, 0x2, 0x2, 0x2, 
       0x4c, 0x46, 0x3, 0x2, 0x2, 0x2, 0x4c, 0x49, 0x3, 0x2, 0x2, 0x2, 0x4d, 
       0x9, 0x3, 0x2, 0x2, 0x2, 0x4e, 0x4f, 0x5, 0x10, 0x9, 0x2, 0x4f, 0x50, 
       0x7, 0x10, 0x2, 0x2, 0x50, 0xb, 0x3, 0x2, 0x2, 0x2, 0x51, 0x52, 0x5, 
       0x6, 0x4, 0x2, 0x52, 0x53, 0x7, 0x17, 0x2, 0x2, 0x53, 0x54, 0x5, 
       0xe, 0x8, 0x2, 0x54, 0x56, 0x7, 0x11, 0x2, 0x2, 0x55, 0x57, 0x5, 
       0x14, 0xb, 0x2, 0x56, 0x55, 0x3, 0x2, 0x2, 0x2, 0x56, 0x57, 0x3, 
       0x2, 0x2, 0x2, 0x57, 0x58, 0x3, 0x2, 0x2, 0x2, 0x58, 0x59, 0x7, 0x12, 
       0x2, 0x2, 0x59, 0x5a, 0x7, 0x10, 0x2, 0x2, 0x5a, 0xd, 0x3, 0x2, 0x2, 
       0x2, 0x5b, 0x5c, 0x9, 0x2, 0x2, 0x2, 0x5c, 0xf, 0x3, 0x2, 0x2, 0x2, 
       0x5d, 0x5e, 0x5, 0x6, 0x4, 0x2, 0x5e, 0x5f, 0x7, 0x17, 0x2, 0x2, 
       0x5f, 0x60, 0x7, 0xc, 0x2, 0x2, 0x60, 0x61, 0x7, 0x11, 0x2, 0x2, 
       0x61, 0x62, 0x5, 0x16, 0xc, 0x2, 0x62, 0x63, 0x7, 0x12, 0x2, 0x2, 
       0x63, 0x64, 0x7, 0x10, 0x2, 0x2, 0x64, 0x11, 0x3, 0x2, 0x2, 0x2, 
       0x65, 0x66, 0x5, 0x6, 0x4, 0x2, 0x66, 0x67, 0x7, 0x17, 0x2, 0x2, 
       0x67, 0x68, 0x7, 0xd, 0x2, 0x2, 0x68, 0x6a, 0x7, 0x11, 0x2, 0x2, 
       0x69, 0x6b, 0x5, 0x14, 0xb, 0x2, 0x6a, 0x69, 0x3, 0x2, 0x2, 0x2, 
       0x6a, 0x6b, 0x3, 0x2, 0x2, 0x2, 0x6b, 0x6c, 0x3, 0x2, 0x2, 0x2, 0x6c, 
       0x6d, 0x7, 0x12, 0x2, 0x2, 0x6d, 0x6e, 0x7, 0x10, 0x2, 0x2, 0x6e, 
       0x13, 0x3, 0x2, 0x2, 0x2, 0x6f, 0x70, 0x5, 0x18, 0xd, 0x2, 0x70, 
       0x71, 0x7, 0x16, 0x2, 0x2, 0x71, 0x73, 0x3, 0x2, 0x2, 0x2, 0x72, 
       0x6f, 0x3, 0x2, 0x2, 0x2, 0x73, 0x76, 0x3, 0x2, 0x2, 0x2, 0x74, 0x72, 
       0x3, 0x2, 0x2, 0x2, 0x74, 0x75, 0x3, 0x2, 0x2, 0x2, 0x75, 0x77, 0x3, 
       0x2, 0x2, 0x2, 0x76, 0x74, 0x3, 0x2, 0x2, 0x2, 0x77, 0x78, 0x5, 0x18, 
       0xd, 0x2, 0x78, 0x15, 0x3, 0x2, 0x2, 0x2, 0x79, 0x7e, 0x5, 0x1e, 
       0x10, 0x2, 0x7a, 0x7b, 0x7, 0x16, 0x2, 0x2, 0x7b, 0x7d, 0x5, 0x1e, 
       0x10, 0x2, 0x7c, 0x7a, 0x3, 0x2, 0x2, 0x2, 0x7d, 0x80, 0x3, 0x2, 
       0x2, 0x2, 0x7e, 0x7c, 0x3, 0x2, 0x2, 0x2, 0x7e, 0x7f, 0x3, 0x2, 0x2, 
       0x2, 0x7f, 0x17, 0x3, 0x2, 0x2, 0x2, 0x80, 0x7e, 0x3, 0x2, 0x2, 0x2, 
       0x81, 0x82, 0x5, 0x1a, 0xe, 0x2, 0x82, 0x19, 0x3, 0x2, 0x2, 0x2, 
       0x83, 0x8c, 0x7, 0x1e, 0x2, 0x2, 0x84, 0x8c, 0x7, 0x1d, 0x2, 0x2, 
       0x85, 0x8c, 0x7, 0x1f, 0x2, 0x2, 0x86, 0x8c, 0x7, 0x21, 0x2, 0x2, 
       0x87, 0x8c, 0x7, 0x20, 0x2, 0x2, 0x88, 0x8c, 0x5, 0x1c, 0xf, 0x2, 
       0x89, 0x8a, 0x7, 0x1a, 0x2, 0x2, 0x8a, 0x8c, 0x5, 0x1a, 0xe, 0x2, 
       0x8b, 0x83, 0x3, 0x2, 0x2, 0x2, 0x8b, 0x84, 0x3, 0x2, 0x2, 0x2, 0x8b, 
       0x85, 0x3, 0x2, 0x2, 0x2, 0x8b, 0x86, 0x3, 0x2, 0x2, 0x2, 0x8b, 0x87, 
       0x3, 0x2, 0x2, 0x2, 0x8b, 0x88, 0x3, 0x2, 0x2, 0x2, 0x8b, 0x89, 0x3, 
       0x2, 0x2, 0x2, 0x8c, 0x1b, 0x3, 0x2, 0x2, 0x2, 0x8d, 0x92, 0x7, 0x13, 
       0x2, 0x2, 0x8e, 0x8f, 0x7, 0x1d, 0x2, 0x2, 0x8f, 0x91, 0x7, 0x16, 
       0x2, 0x2, 0x90, 0x8e, 0x3, 0x2, 0x2, 0x2, 0x91, 0x94, 0x3, 0x2, 0x2, 
       0x2, 0x92, 0x90, 0x3, 0x2, 0x2, 0x2, 0x92, 0x93, 0x3, 0x2, 0x2, 0x2, 
       0x93, 0x95, 0x3, 0x2, 0x2, 0x2, 0x94, 0x92, 0x3, 0x2, 0x2, 0x2, 0x95, 
       0x96, 0x7, 0x1d, 0x2, 0x2, 0x96, 0x97, 0x7, 0x14, 0x2, 0x2, 0x97, 
       0x1d, 0x3, 0x2, 0x2, 0x2, 0x98, 0x9b, 0x7, 0x1d, 0x2, 0x2, 0x99, 
       0x9b, 0x5, 0x2c, 0x17, 0x2, 0x9a, 0x98, 0x3, 0x2, 0x2, 0x2, 0x9a, 
       0x99, 0x3, 0x2, 0x2, 0x2, 0x9b, 0x1f, 0x3, 0x2, 0x2, 0x2, 0x9c, 0xa1, 
       0x5, 0xe, 0x8, 0x2, 0x9d, 0xa1, 0x7, 0xc, 0x2, 0x2, 0x9e, 0xa1, 0x7, 
       0xd, 0x2, 0x2, 0x9f, 0xa1, 0x7, 0x21, 0x2, 0x2, 0xa0, 0x9c, 0x3, 
       0x2, 0x2, 0x2, 0xa0, 0x9d, 0x3, 0x2, 0x2, 0x2, 0xa0, 0x9e, 0x3, 0x2, 
       0x2, 0x2, 0xa0, 0x9f, 0x3, 0x2, 0x2, 0x2, 0xa1, 0x21, 0x3, 0x2, 0x2, 
       0x2, 0xa2, 0xa3, 0x8, 0x12, 0x1, 0x2, 0xa3, 0xa4, 0x5, 0x24, 0x13, 
       0x2, 0xa4, 0xad, 0x3, 0x2, 0x2, 0x2, 0xa5, 0xa6, 0xc, 0x4, 0x2, 0x2, 
       0xa6, 0xa7, 0x7, 0x19, 0x2, 0x2, 0xa7, 0xac, 0x5, 0x24, 0x13, 0x2, 
       0xa8, 0xa9, 0xc, 0x3, 0x2, 0x2, 0xa9, 0xaa, 0x7, 0x1a, 0x2, 0x2, 
       0xaa, 0xac, 0x5, 0x24, 0x13, 0x2, 0xab, 0xa5, 0x3, 0x2, 0x2, 0x2, 
       0xab, 0xa8, 0x3, 0x2, 0x2, 0x2, 0xac, 0xaf, 0x3, 0x2, 0x2, 0x2, 0xad, 
       0xab, 0x3, 0x2, 0x2, 0x2, 0xad, 0xae, 0x3, 0x2, 0x2, 0x2, 0xae, 0x23, 
       0x3, 0x2, 0x2, 0x2, 0xaf, 0xad, 0x3, 0x2, 0x2, 0x2, 0xb0, 0xb1, 0x8, 
       0x13, 0x1, 0x2, 0xb1, 0xb2, 0x5, 0x26, 0x14, 0x2, 0xb2, 0xbb, 0x3, 
       0x2, 0x2, 0x2, 0xb3, 0xb4, 0xc, 0x4, 0x2, 0x2, 0xb4, 0xb5, 0x7, 0x1b, 
       0x2, 0x2, 0xb5, 0xba, 0x5, 0x26, 0x14, 0x2, 0xb6, 0xb7, 0xc, 0x3, 
       0x2, 0x2, 0xb7, 0xb8, 0x7, 0x1c, 0x2, 0x2, 0xb8, 0xba, 0x5, 0x26, 
       0x14, 0x2, 0xb9, 0xb3, 0x3, 0x2, 0x2, 0x2, 0xb9, 0xb6, 0x3, 0x2, 
       0x2, 0x2, 0xba, 0xbd, 0x3, 0x2, 0x2, 0x2, 0xbb, 0xb9, 0x3, 0x2, 0x2, 
       0x2, 0xbb, 0xbc, 0x3, 0x2, 0x2, 0x2, 0xbc, 0x25, 0x3, 0x2, 0x2, 0x2, 
       0xbd, 0xbb, 0x3, 0x2, 0x2, 0x2, 0xbe, 0xc6, 0x7, 0x1d, 0x2, 0x2, 
       0xbf, 0xc0, 0x7, 0x11, 0x2, 0x2, 0xc0, 0xc1, 0x5, 0x22, 0x12, 0x2, 
       0xc1, 0xc2, 0x7, 0x12, 0x2, 0x2, 0xc2, 0xc6, 0x3, 0x2, 0x2, 0x2, 
       0xc3, 0xc6, 0x5, 0x2c, 0x17, 0x2, 0xc4, 0xc6, 0x5, 0x2e, 0x18, 0x2, 
       0xc5, 0xbe, 0x3, 0x2, 0x2, 0x2, 0xc5, 0xbf, 0x3, 0x2, 0x2, 0x2, 0xc5, 
       0xc3, 0x3, 0x2, 0x2, 0x2, 0xc5, 0xc4, 0x3, 0x2, 0x2, 0x2, 0xc6, 0x27, 
       0x3, 0x2, 0x2, 0x2, 0xc7, 0xc8, 0x7, 0x1d, 0x2, 0x2, 0xc8, 0x29, 
       0x3, 0x2, 0x2, 0x2, 0xc9, 0xca, 0x7, 0x1d, 0x2, 0x2, 0xca, 0x2b, 
       0x3, 0x2, 0x2, 0x2, 0xcb, 0xcc, 0x7, 0xe, 0x2, 0x2, 0xcc, 0xcd, 0x7, 
       0x13, 0x2, 0x2, 0xcd, 0xce, 0x7, 0x1d, 0x2, 0x2, 0xce, 0xcf, 0x7, 
       0x14, 0x2, 0x2, 0xcf, 0x2d, 0x3, 0x2, 0x2, 0x2, 0xd0, 0xd1, 0x7, 
       0xf, 0x2, 0x2, 0xd1, 0xd2, 0x7, 0x13, 0x2, 0x2, 0xd2, 0xd3, 0x7, 
       0x1d, 0x2, 0x2, 0xd3, 0xd4, 0x7, 0x14, 0x2, 0x2, 0xd4, 0x2f, 0x3, 
       0x2, 0x2, 0x2, 0x13, 0x33, 0x35, 0x3f, 0x4c, 0x56, 0x6a, 0x74, 0x7e, 
       0x8b, 0x92, 0x9a, 0xa0, 0xab, 0xad, 0xb9, 0xbb, 0xc5, 
  };

  _serializedATN.insert(_serializedATN.end(), serializedATNSegment0,
    serializedATNSegment0 + sizeof(serializedATNSegment0) / sizeof(serializedATNSegment0[0]));


  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

qiskitParser::Initializer qiskitParser::_init;
