
// Generated from qcis.g4 by ANTLR 4.9.2


#include "qcisListener.h"
#include "qcisVisitor.h"

#include "qcisParser.h"


using namespace antlrcpp;
using namespace qcis;
using namespace antlr4;

qcisParser::qcisParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

qcisParser::~qcisParser() {
  delete _interpreter;
}

std::string qcisParser::getGrammarFileName() const {
  return "qcis.g4";
}

const std::vector<std::string>& qcisParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& qcisParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- ProgramContext ------------------------------------------------------------------

qcisParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qcisParser::QuantumCircuitDeclarationContext* qcisParser::ProgramContext::quantumCircuitDeclaration() {
  return getRuleContext<qcisParser::QuantumCircuitDeclarationContext>(0);
}

std::vector<qcisParser::QuantumStatementContext *> qcisParser::ProgramContext::quantumStatement() {
  return getRuleContexts<qcisParser::QuantumStatementContext>();
}

qcisParser::QuantumStatementContext* qcisParser::ProgramContext::quantumStatement(size_t i) {
  return getRuleContext<qcisParser::QuantumStatementContext>(i);
}

std::vector<qcisParser::MeasurementStatementContext *> qcisParser::ProgramContext::measurementStatement() {
  return getRuleContexts<qcisParser::MeasurementStatementContext>();
}

qcisParser::MeasurementStatementContext* qcisParser::ProgramContext::measurementStatement(size_t i) {
  return getRuleContext<qcisParser::MeasurementStatementContext>(i);
}


size_t qcisParser::ProgramContext::getRuleIndex() const {
  return qcisParser::RuleProgram;
}

void qcisParser::ProgramContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProgram(this);
}

void qcisParser::ProgramContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProgram(this);
}


antlrcpp::Any qcisParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::ProgramContext* qcisParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 0, qcisParser::RuleProgram);
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
    // Must match qcis.g4: program : (quantumStatement | measurementStatement)* ;
    // Do not parse quantumCircuitDeclaration here; the generated code was out of
    // sync with the grammar and consumed the first gate as a bogus circuit name.
    setState(42);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << qcisParser::T__1)
      | (1ULL << qcisParser::T__2)
      | (1ULL << qcisParser::Identifier))) != 0)) {
      setState(45);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case qcisParser::T__1:
        case qcisParser::Identifier: {
          setState(43);
          quantumStatement();
          break;
        }

        case qcisParser::T__2: {
          setState(44);
          measurementStatement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(49);
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

qcisParser::QuantumCircuitDeclarationContext::QuantumCircuitDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qcisParser::CircuitNameContext* qcisParser::QuantumCircuitDeclarationContext::circuitName() {
  return getRuleContext<qcisParser::CircuitNameContext>(0);
}

tree::TerminalNode* qcisParser::QuantumCircuitDeclarationContext::EQUALS() {
  return getToken(qcisParser::EQUALS, 0);
}

tree::TerminalNode* qcisParser::QuantumCircuitDeclarationContext::LPAREN() {
  return getToken(qcisParser::LPAREN, 0);
}

qcisParser::QuantumCountContext* qcisParser::QuantumCircuitDeclarationContext::quantumCount() {
  return getRuleContext<qcisParser::QuantumCountContext>(0);
}

tree::TerminalNode* qcisParser::QuantumCircuitDeclarationContext::RPAREN() {
  return getToken(qcisParser::RPAREN, 0);
}

tree::TerminalNode* qcisParser::QuantumCircuitDeclarationContext::SEMICOLON() {
  return getToken(qcisParser::SEMICOLON, 0);
}

tree::TerminalNode* qcisParser::QuantumCircuitDeclarationContext::COMMA() {
  return getToken(qcisParser::COMMA, 0);
}

qcisParser::ClassicalCountContext* qcisParser::QuantumCircuitDeclarationContext::classicalCount() {
  return getRuleContext<qcisParser::ClassicalCountContext>(0);
}


size_t qcisParser::QuantumCircuitDeclarationContext::getRuleIndex() const {
  return qcisParser::RuleQuantumCircuitDeclaration;
}

void qcisParser::QuantumCircuitDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumCircuitDeclaration(this);
}

void qcisParser::QuantumCircuitDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumCircuitDeclaration(this);
}


antlrcpp::Any qcisParser::QuantumCircuitDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQuantumCircuitDeclaration(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QuantumCircuitDeclarationContext* qcisParser::quantumCircuitDeclaration() {
  QuantumCircuitDeclarationContext *_localctx = _tracker.createInstance<QuantumCircuitDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 2, qcisParser::RuleQuantumCircuitDeclaration);
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
    setState(50);
    circuitName();
    setState(51);
    match(qcisParser::EQUALS);
    setState(52);
    match(qcisParser::T__0);
    setState(53);
    match(qcisParser::LPAREN);
    setState(54);
    quantumCount();
    setState(57);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == qcisParser::COMMA) {
      setState(55);
      match(qcisParser::COMMA);
      setState(56);
      classicalCount();
    }
    setState(59);
    match(qcisParser::RPAREN);
    setState(60);
    match(qcisParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CircuitNameContext ------------------------------------------------------------------

qcisParser::CircuitNameContext::CircuitNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::CircuitNameContext::Identifier() {
  return getToken(qcisParser::Identifier, 0);
}


size_t qcisParser::CircuitNameContext::getRuleIndex() const {
  return qcisParser::RuleCircuitName;
}

void qcisParser::CircuitNameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCircuitName(this);
}

void qcisParser::CircuitNameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCircuitName(this);
}


antlrcpp::Any qcisParser::CircuitNameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitCircuitName(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::CircuitNameContext* qcisParser::circuitName() {
  CircuitNameContext *_localctx = _tracker.createInstance<CircuitNameContext>(_ctx, getState());
  enterRule(_localctx, 4, qcisParser::RuleCircuitName);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(62);
    match(qcisParser::Identifier);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumStatementContext ------------------------------------------------------------------

qcisParser::QuantumStatementContext::QuantumStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qcisParser::QuantumGateCallContext* qcisParser::QuantumStatementContext::quantumGateCall() {
  return getRuleContext<qcisParser::QuantumGateCallContext>(0);
}

qcisParser::QuantumBarrierContext* qcisParser::QuantumStatementContext::quantumBarrier() {
  return getRuleContext<qcisParser::QuantumBarrierContext>(0);
}

tree::TerminalNode* qcisParser::QuantumStatementContext::SEMICOLON() {
  return getToken(qcisParser::SEMICOLON, 0);
}


size_t qcisParser::QuantumStatementContext::getRuleIndex() const {
  return qcisParser::RuleQuantumStatement;
}

void qcisParser::QuantumStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumStatement(this);
}

void qcisParser::QuantumStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumStatement(this);
}


antlrcpp::Any qcisParser::QuantumStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQuantumStatement(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QuantumStatementContext* qcisParser::quantumStatement() {
  QuantumStatementContext *_localctx = _tracker.createInstance<QuantumStatementContext>(_ctx, getState());
  enterRule(_localctx, 6, qcisParser::RuleQuantumStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(68);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qcisParser::Identifier: {
        enterOuterAlt(_localctx, 1);
        setState(64);
        quantumGateCall();
        break;
      }

      case qcisParser::T__1: {
        enterOuterAlt(_localctx, 2);
        setState(65);
        quantumBarrier();
        setState(66);
        match(qcisParser::SEMICOLON);
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

//----------------- MeasurementStatementContext ------------------------------------------------------------------

qcisParser::MeasurementStatementContext::MeasurementStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qcisParser::QuantumMeasurementContext* qcisParser::MeasurementStatementContext::quantumMeasurement() {
  return getRuleContext<qcisParser::QuantumMeasurementContext>(0);
}

tree::TerminalNode* qcisParser::MeasurementStatementContext::SEMICOLON() {
  return getToken(qcisParser::SEMICOLON, 0);
}


size_t qcisParser::MeasurementStatementContext::getRuleIndex() const {
  return qcisParser::RuleMeasurementStatement;
}

void qcisParser::MeasurementStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMeasurementStatement(this);
}

void qcisParser::MeasurementStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMeasurementStatement(this);
}


antlrcpp::Any qcisParser::MeasurementStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitMeasurementStatement(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::MeasurementStatementContext* qcisParser::measurementStatement() {
  MeasurementStatementContext *_localctx = _tracker.createInstance<MeasurementStatementContext>(_ctx, getState());
  enterRule(_localctx, 8, qcisParser::RuleMeasurementStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(70);
    quantumMeasurement();
    setState(71);
    match(qcisParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumGateCallContext ------------------------------------------------------------------

qcisParser::QuantumGateCallContext::QuantumGateCallContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qcisParser::QuantumGateNameContext* qcisParser::QuantumGateCallContext::quantumGateName() {
  return getRuleContext<qcisParser::QuantumGateNameContext>(0);
}

std::vector<tree::TerminalNode *> qcisParser::QuantumGateCallContext::QubitId() {
  return getTokens(qcisParser::QubitId);
}

tree::TerminalNode* qcisParser::QuantumGateCallContext::QubitId(size_t i) {
  return getToken(qcisParser::QubitId, i);
}

qcisParser::GateArgumentListContext* qcisParser::QuantumGateCallContext::gateArgumentList() {
  return getRuleContext<qcisParser::GateArgumentListContext>(0);
}


size_t qcisParser::QuantumGateCallContext::getRuleIndex() const {
  return qcisParser::RuleQuantumGateCall;
}

void qcisParser::QuantumGateCallContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumGateCall(this);
}

void qcisParser::QuantumGateCallContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumGateCall(this);
}


antlrcpp::Any qcisParser::QuantumGateCallContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQuantumGateCall(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QuantumGateCallContext* qcisParser::quantumGateCall() {
  QuantumGateCallContext *_localctx = _tracker.createInstance<QuantumGateCallContext>(_ctx, getState());
  enterRule(_localctx, 10, qcisParser::RuleQuantumGateCall);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(73);
    quantumGateName();
    setState(74);
    match(qcisParser::QubitId);
    setState(77);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 4, _ctx)) {
    case 1: {
      setState(75);
      gateArgumentList();
      break;
    }

    case 2: {
      setState(76);
      match(qcisParser::QubitId);
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

//----------------- QuantumBarrierContext ------------------------------------------------------------------

qcisParser::QuantumBarrierContext::QuantumBarrierContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> qcisParser::QuantumBarrierContext::QubitId() {
  return getTokens(qcisParser::QubitId);
}

tree::TerminalNode* qcisParser::QuantumBarrierContext::QubitId(size_t i) {
  return getToken(qcisParser::QubitId, i);
}


size_t qcisParser::QuantumBarrierContext::getRuleIndex() const {
  return qcisParser::RuleQuantumBarrier;
}

void qcisParser::QuantumBarrierContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumBarrier(this);
}

void qcisParser::QuantumBarrierContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumBarrier(this);
}


antlrcpp::Any qcisParser::QuantumBarrierContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQuantumBarrier(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QuantumBarrierContext* qcisParser::quantumBarrier() {
  QuantumBarrierContext *_localctx = _tracker.createInstance<QuantumBarrierContext>(_ctx, getState());
  enterRule(_localctx, 12, qcisParser::RuleQuantumBarrier);
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
    match(qcisParser::T__1);
    setState(81); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(80);
      match(qcisParser::QubitId);
      setState(83); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == qcisParser::QubitId);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumGateNameContext ------------------------------------------------------------------

qcisParser::QuantumGateNameContext::QuantumGateNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::QuantumGateNameContext::Identifier() {
  return getToken(qcisParser::Identifier, 0);
}


size_t qcisParser::QuantumGateNameContext::getRuleIndex() const {
  return qcisParser::RuleQuantumGateName;
}

void qcisParser::QuantumGateNameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumGateName(this);
}

void qcisParser::QuantumGateNameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumGateName(this);
}


antlrcpp::Any qcisParser::QuantumGateNameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQuantumGateName(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QuantumGateNameContext* qcisParser::quantumGateName() {
  QuantumGateNameContext *_localctx = _tracker.createInstance<QuantumGateNameContext>(_ctx, getState());
  enterRule(_localctx, 14, qcisParser::RuleQuantumGateName);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(85);
    match(qcisParser::Identifier);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuantumMeasurementContext ------------------------------------------------------------------

qcisParser::QuantumMeasurementContext::QuantumMeasurementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qcisParser::MeasurementArgumentListContext* qcisParser::QuantumMeasurementContext::measurementArgumentList() {
  return getRuleContext<qcisParser::MeasurementArgumentListContext>(0);
}


size_t qcisParser::QuantumMeasurementContext::getRuleIndex() const {
  return qcisParser::RuleQuantumMeasurement;
}

void qcisParser::QuantumMeasurementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumMeasurement(this);
}

void qcisParser::QuantumMeasurementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumMeasurement(this);
}


antlrcpp::Any qcisParser::QuantumMeasurementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQuantumMeasurement(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QuantumMeasurementContext* qcisParser::quantumMeasurement() {
  QuantumMeasurementContext *_localctx = _tracker.createInstance<QuantumMeasurementContext>(_ctx, getState());
  enterRule(_localctx, 16, qcisParser::RuleQuantumMeasurement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(87);
    match(qcisParser::T__2);
    setState(88);
    measurementArgumentList();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GateArgumentListContext ------------------------------------------------------------------

qcisParser::GateArgumentListContext::GateArgumentListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<qcisParser::ExpressionContext *> qcisParser::GateArgumentListContext::expression() {
  return getRuleContexts<qcisParser::ExpressionContext>();
}

qcisParser::ExpressionContext* qcisParser::GateArgumentListContext::expression(size_t i) {
  return getRuleContext<qcisParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> qcisParser::GateArgumentListContext::COMMA() {
  return getTokens(qcisParser::COMMA);
}

tree::TerminalNode* qcisParser::GateArgumentListContext::COMMA(size_t i) {
  return getToken(qcisParser::COMMA, i);
}


size_t qcisParser::GateArgumentListContext::getRuleIndex() const {
  return qcisParser::RuleGateArgumentList;
}

void qcisParser::GateArgumentListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterGateArgumentList(this);
}

void qcisParser::GateArgumentListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitGateArgumentList(this);
}


antlrcpp::Any qcisParser::GateArgumentListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitGateArgumentList(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::GateArgumentListContext* qcisParser::gateArgumentList() {
  GateArgumentListContext *_localctx = _tracker.createInstance<GateArgumentListContext>(_ctx, getState());
  enterRule(_localctx, 18, qcisParser::RuleGateArgumentList);

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
    setState(95);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(90);
        expression();
        setState(91);
        match(qcisParser::COMMA); 
      }
      setState(97);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx);
    }
    setState(98);
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

qcisParser::MeasurementArgumentListContext::MeasurementArgumentListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::MeasurementArgumentListContext::QubitId() {
  return getToken(qcisParser::QubitId, 0);
}


size_t qcisParser::MeasurementArgumentListContext::getRuleIndex() const {
  return qcisParser::RuleMeasurementArgumentList;
}

void qcisParser::MeasurementArgumentListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMeasurementArgumentList(this);
}

void qcisParser::MeasurementArgumentListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMeasurementArgumentList(this);
}


antlrcpp::Any qcisParser::MeasurementArgumentListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitMeasurementArgumentList(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::MeasurementArgumentListContext* qcisParser::measurementArgumentList() {
  MeasurementArgumentListContext *_localctx = _tracker.createInstance<MeasurementArgumentListContext>(_ctx, getState());
  enterRule(_localctx, 20, qcisParser::RuleMeasurementArgumentList);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(100);
    match(qcisParser::QubitId);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GateArgumentContext ------------------------------------------------------------------

qcisParser::GateArgumentContext::GateArgumentContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t qcisParser::GateArgumentContext::getRuleIndex() const {
  return qcisParser::RuleGateArgument;
}

void qcisParser::GateArgumentContext::copyFrom(GateArgumentContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpressionArgumentContext ------------------------------------------------------------------

qcisParser::ExpressionContext* qcisParser::ExpressionArgumentContext::expression() {
  return getRuleContext<qcisParser::ExpressionContext>(0);
}

qcisParser::ExpressionArgumentContext::ExpressionArgumentContext(GateArgumentContext *ctx) { copyFrom(ctx); }

void qcisParser::ExpressionArgumentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpressionArgument(this);
}
void qcisParser::ExpressionArgumentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpressionArgument(this);
}

antlrcpp::Any qcisParser::ExpressionArgumentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitExpressionArgument(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IntegerGateArgumentContext ------------------------------------------------------------------

tree::TerminalNode* qcisParser::IntegerGateArgumentContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}

qcisParser::IntegerGateArgumentContext::IntegerGateArgumentContext(GateArgumentContext *ctx) { copyFrom(ctx); }

void qcisParser::IntegerGateArgumentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntegerGateArgument(this);
}
void qcisParser::IntegerGateArgumentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntegerGateArgument(this);
}

antlrcpp::Any qcisParser::IntegerGateArgumentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitIntegerGateArgument(this);
  else
    return visitor->visitChildren(this);
}
qcisParser::GateArgumentContext* qcisParser::gateArgument() {
  GateArgumentContext *_localctx = _tracker.createInstance<GateArgumentContext>(_ctx, getState());
  enterRule(_localctx, 22, qcisParser::RuleGateArgument);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(104);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 7, _ctx)) {
    case 1: {
      _localctx = dynamic_cast<GateArgumentContext *>(_tracker.createInstance<qcisParser::ExpressionArgumentContext>(_localctx));
      enterOuterAlt(_localctx, 1);
      setState(102);
      expression();
      break;
    }

    case 2: {
      _localctx = dynamic_cast<GateArgumentContext *>(_tracker.createInstance<qcisParser::IntegerGateArgumentContext>(_localctx));
      enterOuterAlt(_localctx, 2);
      setState(103);
      match(qcisParser::Integer);
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

//----------------- ExpressionContext ------------------------------------------------------------------

qcisParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

qcisParser::ExpressionTerminatorContext* qcisParser::ExpressionContext::expressionTerminator() {
  return getRuleContext<qcisParser::ExpressionTerminatorContext>(0);
}


size_t qcisParser::ExpressionContext::getRuleIndex() const {
  return qcisParser::RuleExpression;
}

void qcisParser::ExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpression(this);
}

void qcisParser::ExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpression(this);
}


antlrcpp::Any qcisParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::ExpressionContext* qcisParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 24, qcisParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(106);
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

qcisParser::ExpressionTerminatorContext::ExpressionTerminatorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::ExpressionTerminatorContext::Constant() {
  return getToken(qcisParser::Constant, 0);
}

tree::TerminalNode* qcisParser::ExpressionTerminatorContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}

tree::TerminalNode* qcisParser::ExpressionTerminatorContext::RealNumber() {
  return getToken(qcisParser::RealNumber, 0);
}

tree::TerminalNode* qcisParser::ExpressionTerminatorContext::Identifier() {
  return getToken(qcisParser::Identifier, 0);
}

tree::TerminalNode* qcisParser::ExpressionTerminatorContext::StringLiteral() {
  return getToken(qcisParser::StringLiteral, 0);
}

qcisParser::ListExpressionContext* qcisParser::ExpressionTerminatorContext::listExpression() {
  return getRuleContext<qcisParser::ListExpressionContext>(0);
}

tree::TerminalNode* qcisParser::ExpressionTerminatorContext::MINUS() {
  return getToken(qcisParser::MINUS, 0);
}

qcisParser::ExpressionTerminatorContext* qcisParser::ExpressionTerminatorContext::expressionTerminator() {
  return getRuleContext<qcisParser::ExpressionTerminatorContext>(0);
}


size_t qcisParser::ExpressionTerminatorContext::getRuleIndex() const {
  return qcisParser::RuleExpressionTerminator;
}

void qcisParser::ExpressionTerminatorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpressionTerminator(this);
}

void qcisParser::ExpressionTerminatorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpressionTerminator(this);
}


antlrcpp::Any qcisParser::ExpressionTerminatorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitExpressionTerminator(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::ExpressionTerminatorContext* qcisParser::expressionTerminator() {
  ExpressionTerminatorContext *_localctx = _tracker.createInstance<ExpressionTerminatorContext>(_ctx, getState());
  enterRule(_localctx, 26, qcisParser::RuleExpressionTerminator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(116);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qcisParser::Constant: {
        enterOuterAlt(_localctx, 1);
        setState(108);
        match(qcisParser::Constant);
        break;
      }

      case qcisParser::Integer: {
        enterOuterAlt(_localctx, 2);
        setState(109);
        match(qcisParser::Integer);
        break;
      }

      case qcisParser::RealNumber: {
        enterOuterAlt(_localctx, 3);
        setState(110);
        match(qcisParser::RealNumber);
        break;
      }

      case qcisParser::Identifier: {
        enterOuterAlt(_localctx, 4);
        setState(111);
        match(qcisParser::Identifier);
        break;
      }

      case qcisParser::StringLiteral: {
        enterOuterAlt(_localctx, 5);
        setState(112);
        match(qcisParser::StringLiteral);
        break;
      }

      case qcisParser::LBRACKET: {
        enterOuterAlt(_localctx, 6);
        setState(113);
        listExpression();
        break;
      }

      case qcisParser::MINUS: {
        enterOuterAlt(_localctx, 7);
        setState(114);
        match(qcisParser::MINUS);
        setState(115);
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

qcisParser::ListExpressionContext::ListExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::ListExpressionContext::LBRACKET() {
  return getToken(qcisParser::LBRACKET, 0);
}

std::vector<tree::TerminalNode *> qcisParser::ListExpressionContext::Integer() {
  return getTokens(qcisParser::Integer);
}

tree::TerminalNode* qcisParser::ListExpressionContext::Integer(size_t i) {
  return getToken(qcisParser::Integer, i);
}

tree::TerminalNode* qcisParser::ListExpressionContext::RBRACKET() {
  return getToken(qcisParser::RBRACKET, 0);
}

std::vector<tree::TerminalNode *> qcisParser::ListExpressionContext::COMMA() {
  return getTokens(qcisParser::COMMA);
}

tree::TerminalNode* qcisParser::ListExpressionContext::COMMA(size_t i) {
  return getToken(qcisParser::COMMA, i);
}


size_t qcisParser::ListExpressionContext::getRuleIndex() const {
  return qcisParser::RuleListExpression;
}

void qcisParser::ListExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterListExpression(this);
}

void qcisParser::ListExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitListExpression(this);
}


antlrcpp::Any qcisParser::ListExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitListExpression(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::ListExpressionContext* qcisParser::listExpression() {
  ListExpressionContext *_localctx = _tracker.createInstance<ListExpressionContext>(_ctx, getState());
  enterRule(_localctx, 28, qcisParser::RuleListExpression);

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
    setState(118);
    match(qcisParser::LBRACKET);
    setState(123);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(119);
        match(qcisParser::Integer);
        setState(120);
        match(qcisParser::COMMA); 
      }
      setState(125);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx);
    }
    setState(126);
    match(qcisParser::Integer);
    setState(127);
    match(qcisParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MeasurementArgumentContext ------------------------------------------------------------------

qcisParser::MeasurementArgumentContext::MeasurementArgumentContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t qcisParser::MeasurementArgumentContext::getRuleIndex() const {
  return qcisParser::RuleMeasurementArgument;
}

void qcisParser::MeasurementArgumentContext::copyFrom(MeasurementArgumentContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- QubitMeasurementArgumentContext ------------------------------------------------------------------

qcisParser::QubitReferenceContext* qcisParser::QubitMeasurementArgumentContext::qubitReference() {
  return getRuleContext<qcisParser::QubitReferenceContext>(0);
}

qcisParser::QubitMeasurementArgumentContext::QubitMeasurementArgumentContext(MeasurementArgumentContext *ctx) { copyFrom(ctx); }

void qcisParser::QubitMeasurementArgumentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQubitMeasurementArgument(this);
}
void qcisParser::QubitMeasurementArgumentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQubitMeasurementArgument(this);
}

antlrcpp::Any qcisParser::QubitMeasurementArgumentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQubitMeasurementArgument(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IntegerMeasurementArgumentContext ------------------------------------------------------------------

tree::TerminalNode* qcisParser::IntegerMeasurementArgumentContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}

qcisParser::IntegerMeasurementArgumentContext::IntegerMeasurementArgumentContext(MeasurementArgumentContext *ctx) { copyFrom(ctx); }

void qcisParser::IntegerMeasurementArgumentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntegerMeasurementArgument(this);
}
void qcisParser::IntegerMeasurementArgumentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntegerMeasurementArgument(this);
}

antlrcpp::Any qcisParser::IntegerMeasurementArgumentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitIntegerMeasurementArgument(this);
  else
    return visitor->visitChildren(this);
}
qcisParser::MeasurementArgumentContext* qcisParser::measurementArgument() {
  MeasurementArgumentContext *_localctx = _tracker.createInstance<MeasurementArgumentContext>(_ctx, getState());
  enterRule(_localctx, 30, qcisParser::RuleMeasurementArgument);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(131);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qcisParser::Integer: {
        _localctx = dynamic_cast<MeasurementArgumentContext *>(_tracker.createInstance<qcisParser::IntegerMeasurementArgumentContext>(_localctx));
        enterOuterAlt(_localctx, 1);
        setState(129);
        match(qcisParser::Integer);
        break;
      }

      case qcisParser::T__3:
      case qcisParser::QubitId: {
        _localctx = dynamic_cast<MeasurementArgumentContext *>(_tracker.createInstance<qcisParser::QubitMeasurementArgumentContext>(_localctx));
        enterOuterAlt(_localctx, 2);
        setState(130);
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

//----------------- QuantumCountContext ------------------------------------------------------------------

qcisParser::QuantumCountContext::QuantumCountContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::QuantumCountContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}


size_t qcisParser::QuantumCountContext::getRuleIndex() const {
  return qcisParser::RuleQuantumCount;
}

void qcisParser::QuantumCountContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuantumCount(this);
}

void qcisParser::QuantumCountContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuantumCount(this);
}


antlrcpp::Any qcisParser::QuantumCountContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQuantumCount(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QuantumCountContext* qcisParser::quantumCount() {
  QuantumCountContext *_localctx = _tracker.createInstance<QuantumCountContext>(_ctx, getState());
  enterRule(_localctx, 32, qcisParser::RuleQuantumCount);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(133);
    match(qcisParser::Integer);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassicalCountContext ------------------------------------------------------------------

qcisParser::ClassicalCountContext::ClassicalCountContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::ClassicalCountContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}


size_t qcisParser::ClassicalCountContext::getRuleIndex() const {
  return qcisParser::RuleClassicalCount;
}

void qcisParser::ClassicalCountContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassicalCount(this);
}

void qcisParser::ClassicalCountContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassicalCount(this);
}


antlrcpp::Any qcisParser::ClassicalCountContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitClassicalCount(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::ClassicalCountContext* qcisParser::classicalCount() {
  ClassicalCountContext *_localctx = _tracker.createInstance<ClassicalCountContext>(_ctx, getState());
  enterRule(_localctx, 34, qcisParser::RuleClassicalCount);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(135);
    match(qcisParser::Integer);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QubitReferenceContext ------------------------------------------------------------------

qcisParser::QubitReferenceContext::QubitReferenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::QubitReferenceContext::LBRACKET() {
  return getToken(qcisParser::LBRACKET, 0);
}

tree::TerminalNode* qcisParser::QubitReferenceContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}

tree::TerminalNode* qcisParser::QubitReferenceContext::RBRACKET() {
  return getToken(qcisParser::RBRACKET, 0);
}

tree::TerminalNode* qcisParser::QubitReferenceContext::QubitId() {
  return getToken(qcisParser::QubitId, 0);
}


size_t qcisParser::QubitReferenceContext::getRuleIndex() const {
  return qcisParser::RuleQubitReference;
}

void qcisParser::QubitReferenceContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQubitReference(this);
}

void qcisParser::QubitReferenceContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQubitReference(this);
}


antlrcpp::Any qcisParser::QubitReferenceContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitQubitReference(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::QubitReferenceContext* qcisParser::qubitReference() {
  QubitReferenceContext *_localctx = _tracker.createInstance<QubitReferenceContext>(_ctx, getState());
  enterRule(_localctx, 36, qcisParser::RuleQubitReference);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(142);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qcisParser::T__3: {
        enterOuterAlt(_localctx, 1);
        setState(137);
        match(qcisParser::T__3);
        setState(138);
        match(qcisParser::LBRACKET);
        setState(139);
        match(qcisParser::Integer);
        setState(140);
        match(qcisParser::RBRACKET);
        break;
      }

      case qcisParser::QubitId: {
        enterOuterAlt(_localctx, 2);
        setState(141);
        match(qcisParser::QubitId);
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

//----------------- GateParameterContext ------------------------------------------------------------------

qcisParser::GateParameterContext::GateParameterContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::GateParameterContext::RealNumber() {
  return getToken(qcisParser::RealNumber, 0);
}

tree::TerminalNode* qcisParser::GateParameterContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}

tree::TerminalNode* qcisParser::GateParameterContext::Constant() {
  return getToken(qcisParser::Constant, 0);
}

tree::TerminalNode* qcisParser::GateParameterContext::MINUS() {
  return getToken(qcisParser::MINUS, 0);
}

qcisParser::GateParameterContext* qcisParser::GateParameterContext::gateParameter() {
  return getRuleContext<qcisParser::GateParameterContext>(0);
}


size_t qcisParser::GateParameterContext::getRuleIndex() const {
  return qcisParser::RuleGateParameter;
}

void qcisParser::GateParameterContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterGateParameter(this);
}

void qcisParser::GateParameterContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitGateParameter(this);
}


antlrcpp::Any qcisParser::GateParameterContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitGateParameter(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::GateParameterContext* qcisParser::gateParameter() {
  GateParameterContext *_localctx = _tracker.createInstance<GateParameterContext>(_ctx, getState());
  enterRule(_localctx, 38, qcisParser::RuleGateParameter);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(149);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case qcisParser::RealNumber: {
        enterOuterAlt(_localctx, 1);
        setState(144);
        match(qcisParser::RealNumber);
        break;
      }

      case qcisParser::Integer: {
        enterOuterAlt(_localctx, 2);
        setState(145);
        match(qcisParser::Integer);
        break;
      }

      case qcisParser::Constant: {
        enterOuterAlt(_localctx, 3);
        setState(146);
        match(qcisParser::Constant);
        break;
      }

      case qcisParser::MINUS: {
        enterOuterAlt(_localctx, 4);
        setState(147);
        match(qcisParser::MINUS);
        setState(148);
        gateParameter();
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

//----------------- ClassicalReferenceContext ------------------------------------------------------------------

qcisParser::ClassicalReferenceContext::ClassicalReferenceContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* qcisParser::ClassicalReferenceContext::LBRACKET() {
  return getToken(qcisParser::LBRACKET, 0);
}

tree::TerminalNode* qcisParser::ClassicalReferenceContext::Integer() {
  return getToken(qcisParser::Integer, 0);
}

tree::TerminalNode* qcisParser::ClassicalReferenceContext::RBRACKET() {
  return getToken(qcisParser::RBRACKET, 0);
}


size_t qcisParser::ClassicalReferenceContext::getRuleIndex() const {
  return qcisParser::RuleClassicalReference;
}

void qcisParser::ClassicalReferenceContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassicalReference(this);
}

void qcisParser::ClassicalReferenceContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<qcisListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassicalReference(this);
}


antlrcpp::Any qcisParser::ClassicalReferenceContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<qcisVisitor*>(visitor))
    return parserVisitor->visitClassicalReference(this);
  else
    return visitor->visitChildren(this);
}

qcisParser::ClassicalReferenceContext* qcisParser::classicalReference() {
  ClassicalReferenceContext *_localctx = _tracker.createInstance<ClassicalReferenceContext>(_ctx, getState());
  enterRule(_localctx, 40, qcisParser::RuleClassicalReference);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(151);
    match(qcisParser::T__4);
    setState(152);
    match(qcisParser::LBRACKET);
    setState(153);
    match(qcisParser::Integer);
    setState(154);
    match(qcisParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

// Static vars and initialization.
std::vector<dfa::DFA> qcisParser::_decisionToDFA;
atn::PredictionContextCache qcisParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN qcisParser::_atn;
std::vector<uint16_t> qcisParser::_serializedATN;

std::vector<std::string> qcisParser::_ruleNames = {
  "program", "quantumCircuitDeclaration", "circuitName", "quantumStatement", 
  "measurementStatement", "quantumGateCall", "quantumBarrier", "quantumGateName", 
  "quantumMeasurement", "gateArgumentList", "measurementArgumentList", "gateArgument", 
  "expression", "expressionTerminator", "listExpression", "measurementArgument", 
  "quantumCount", "classicalCount", "qubitReference", "gateParameter", "classicalReference"
};

std::vector<std::string> qcisParser::_literalNames = {
  "", "'QuantumCircuit'", "'B'", "'M'", "'q'", "'c'", "';'", "'('", "')'", 
  "'['", "']'", "':'", "','", "'.'", "'='", "'+'", "'-'", "'*'", "'/'"
};

std::vector<std::string> qcisParser::_symbolicNames = {
  "", "", "", "", "", "", "SEMICOLON", "LPAREN", "RPAREN", "LBRACKET", "RBRACKET", 
  "COLON", "COMMA", "DOT", "EQUALS", "PLUS", "MINUS", "STAR", "SLASH", "Integer", 
  "QubitId", "Constant", "RealNumber", "StringLiteral", "Identifier", "Whitespace", 
  "LineComment"
};

dfa::Vocabulary qcisParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> qcisParser::_tokenNames;

qcisParser::Initializer::Initializer() {
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
       0x3, 0x1c, 0x9f, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 
       0x7, 0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 
       0x4, 0xb, 0x9, 0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 
       0xe, 0x9, 0xe, 0x4, 0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 
       0x9, 0x11, 0x4, 0x12, 0x9, 0x12, 0x4, 0x13, 0x9, 0x13, 0x4, 0x14, 
       0x9, 0x14, 0x4, 0x15, 0x9, 0x15, 0x4, 0x16, 0x9, 0x16, 0x3, 0x2, 
       0x3, 0x2, 0x3, 0x2, 0x7, 0x2, 0x30, 0xa, 0x2, 0xc, 0x2, 0xe, 0x2, 
       0x33, 0xb, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x3, 0x5, 0x3, 0x3c, 0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x4, 0x3, 0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 
       0x5, 0x5, 0x5, 0x47, 0xa, 0x5, 0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 
       0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x5, 0x7, 0x50, 0xa, 0x7, 0x3, 
       0x8, 0x3, 0x8, 0x6, 0x8, 0x54, 0xa, 0x8, 0xd, 0x8, 0xe, 0x8, 0x55, 
       0x3, 0x9, 0x3, 0x9, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xb, 0x3, 
       0xb, 0x3, 0xb, 0x7, 0xb, 0x60, 0xa, 0xb, 0xc, 0xb, 0xe, 0xb, 0x63, 
       0xb, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xc, 0x3, 0xc, 0x3, 0xd, 0x3, 
       0xd, 0x5, 0xd, 0x6b, 0xa, 0xd, 0x3, 0xe, 0x3, 0xe, 0x3, 0xf, 0x3, 
       0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 
       0x5, 0xf, 0x77, 0xa, 0xf, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x7, 0x10, 
       0x7c, 0xa, 0x10, 0xc, 0x10, 0xe, 0x10, 0x7f, 0xb, 0x10, 0x3, 0x10, 
       0x3, 0x10, 0x3, 0x10, 0x3, 0x11, 0x3, 0x11, 0x5, 0x11, 0x86, 0xa, 
       0x11, 0x3, 0x12, 0x3, 0x12, 0x3, 0x13, 0x3, 0x13, 0x3, 0x14, 0x3, 
       0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x5, 0x14, 0x91, 0xa, 0x14, 
       0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x5, 0x15, 
       0x98, 0xa, 0x15, 0x3, 0x16, 0x3, 0x16, 0x3, 0x16, 0x3, 0x16, 0x3, 
       0x16, 0x3, 0x16, 0x2, 0x2, 0x17, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 
       0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 
       0x26, 0x28, 0x2a, 0x2, 0x2, 0x2, 0x9e, 0x2, 0x2c, 0x3, 0x2, 0x2, 
       0x2, 0x4, 0x34, 0x3, 0x2, 0x2, 0x2, 0x6, 0x40, 0x3, 0x2, 0x2, 0x2, 
       0x8, 0x46, 0x3, 0x2, 0x2, 0x2, 0xa, 0x48, 0x3, 0x2, 0x2, 0x2, 0xc, 
       0x4b, 0x3, 0x2, 0x2, 0x2, 0xe, 0x51, 0x3, 0x2, 0x2, 0x2, 0x10, 0x57, 
       0x3, 0x2, 0x2, 0x2, 0x12, 0x59, 0x3, 0x2, 0x2, 0x2, 0x14, 0x61, 0x3, 
       0x2, 0x2, 0x2, 0x16, 0x66, 0x3, 0x2, 0x2, 0x2, 0x18, 0x6a, 0x3, 0x2, 
       0x2, 0x2, 0x1a, 0x6c, 0x3, 0x2, 0x2, 0x2, 0x1c, 0x76, 0x3, 0x2, 0x2, 
       0x2, 0x1e, 0x78, 0x3, 0x2, 0x2, 0x2, 0x20, 0x85, 0x3, 0x2, 0x2, 0x2, 
       0x22, 0x87, 0x3, 0x2, 0x2, 0x2, 0x24, 0x89, 0x3, 0x2, 0x2, 0x2, 0x26, 
       0x90, 0x3, 0x2, 0x2, 0x2, 0x28, 0x97, 0x3, 0x2, 0x2, 0x2, 0x2a, 0x99, 
       0x3, 0x2, 0x2, 0x2, 0x2c, 0x31, 0x5, 0x4, 0x3, 0x2, 0x2d, 0x30, 0x5, 
       0x8, 0x5, 0x2, 0x2e, 0x30, 0x5, 0xa, 0x6, 0x2, 0x2f, 0x2d, 0x3, 0x2, 
       0x2, 0x2, 0x2f, 0x2e, 0x3, 0x2, 0x2, 0x2, 0x30, 0x33, 0x3, 0x2, 0x2, 
       0x2, 0x31, 0x2f, 0x3, 0x2, 0x2, 0x2, 0x31, 0x32, 0x3, 0x2, 0x2, 0x2, 
       0x32, 0x3, 0x3, 0x2, 0x2, 0x2, 0x33, 0x31, 0x3, 0x2, 0x2, 0x2, 0x34, 
       0x35, 0x5, 0x6, 0x4, 0x2, 0x35, 0x36, 0x7, 0x10, 0x2, 0x2, 0x36, 
       0x37, 0x7, 0x3, 0x2, 0x2, 0x37, 0x38, 0x7, 0x9, 0x2, 0x2, 0x38, 0x3b, 
       0x5, 0x22, 0x12, 0x2, 0x39, 0x3a, 0x7, 0xe, 0x2, 0x2, 0x3a, 0x3c, 
       0x5, 0x24, 0x13, 0x2, 0x3b, 0x39, 0x3, 0x2, 0x2, 0x2, 0x3b, 0x3c, 
       0x3, 0x2, 0x2, 0x2, 0x3c, 0x3d, 0x3, 0x2, 0x2, 0x2, 0x3d, 0x3e, 0x7, 
       0xa, 0x2, 0x2, 0x3e, 0x3f, 0x7, 0x8, 0x2, 0x2, 0x3f, 0x5, 0x3, 0x2, 
       0x2, 0x2, 0x40, 0x41, 0x7, 0x1a, 0x2, 0x2, 0x41, 0x7, 0x3, 0x2, 0x2, 
       0x2, 0x42, 0x47, 0x5, 0xc, 0x7, 0x2, 0x43, 0x44, 0x5, 0xe, 0x8, 0x2, 
       0x44, 0x45, 0x7, 0x8, 0x2, 0x2, 0x45, 0x47, 0x3, 0x2, 0x2, 0x2, 0x46, 
       0x42, 0x3, 0x2, 0x2, 0x2, 0x46, 0x43, 0x3, 0x2, 0x2, 0x2, 0x47, 0x9, 
       0x3, 0x2, 0x2, 0x2, 0x48, 0x49, 0x5, 0x12, 0xa, 0x2, 0x49, 0x4a, 
       0x7, 0x8, 0x2, 0x2, 0x4a, 0xb, 0x3, 0x2, 0x2, 0x2, 0x4b, 0x4c, 0x5, 
       0x10, 0x9, 0x2, 0x4c, 0x4f, 0x7, 0x16, 0x2, 0x2, 0x4d, 0x50, 0x5, 
       0x14, 0xb, 0x2, 0x4e, 0x50, 0x7, 0x16, 0x2, 0x2, 0x4f, 0x4d, 0x3, 
       0x2, 0x2, 0x2, 0x4f, 0x4e, 0x3, 0x2, 0x2, 0x2, 0x4f, 0x50, 0x3, 0x2, 
       0x2, 0x2, 0x50, 0xd, 0x3, 0x2, 0x2, 0x2, 0x51, 0x53, 0x7, 0x4, 0x2, 
       0x2, 0x52, 0x54, 0x7, 0x16, 0x2, 0x2, 0x53, 0x52, 0x3, 0x2, 0x2, 
       0x2, 0x54, 0x55, 0x3, 0x2, 0x2, 0x2, 0x55, 0x53, 0x3, 0x2, 0x2, 0x2, 
       0x55, 0x56, 0x3, 0x2, 0x2, 0x2, 0x56, 0xf, 0x3, 0x2, 0x2, 0x2, 0x57, 
       0x58, 0x7, 0x1a, 0x2, 0x2, 0x58, 0x11, 0x3, 0x2, 0x2, 0x2, 0x59, 
       0x5a, 0x7, 0x5, 0x2, 0x2, 0x5a, 0x5b, 0x5, 0x16, 0xc, 0x2, 0x5b, 
       0x13, 0x3, 0x2, 0x2, 0x2, 0x5c, 0x5d, 0x5, 0x1a, 0xe, 0x2, 0x5d, 
       0x5e, 0x7, 0xe, 0x2, 0x2, 0x5e, 0x60, 0x3, 0x2, 0x2, 0x2, 0x5f, 0x5c, 
       0x3, 0x2, 0x2, 0x2, 0x60, 0x63, 0x3, 0x2, 0x2, 0x2, 0x61, 0x5f, 0x3, 
       0x2, 0x2, 0x2, 0x61, 0x62, 0x3, 0x2, 0x2, 0x2, 0x62, 0x64, 0x3, 0x2, 
       0x2, 0x2, 0x63, 0x61, 0x3, 0x2, 0x2, 0x2, 0x64, 0x65, 0x5, 0x1a, 
       0xe, 0x2, 0x65, 0x15, 0x3, 0x2, 0x2, 0x2, 0x66, 0x67, 0x7, 0x16, 
       0x2, 0x2, 0x67, 0x17, 0x3, 0x2, 0x2, 0x2, 0x68, 0x6b, 0x5, 0x1a, 
       0xe, 0x2, 0x69, 0x6b, 0x7, 0x15, 0x2, 0x2, 0x6a, 0x68, 0x3, 0x2, 
       0x2, 0x2, 0x6a, 0x69, 0x3, 0x2, 0x2, 0x2, 0x6b, 0x19, 0x3, 0x2, 0x2, 
       0x2, 0x6c, 0x6d, 0x5, 0x1c, 0xf, 0x2, 0x6d, 0x1b, 0x3, 0x2, 0x2, 
       0x2, 0x6e, 0x77, 0x7, 0x17, 0x2, 0x2, 0x6f, 0x77, 0x7, 0x15, 0x2, 
       0x2, 0x70, 0x77, 0x7, 0x18, 0x2, 0x2, 0x71, 0x77, 0x7, 0x1a, 0x2, 
       0x2, 0x72, 0x77, 0x7, 0x19, 0x2, 0x2, 0x73, 0x77, 0x5, 0x1e, 0x10, 
       0x2, 0x74, 0x75, 0x7, 0x12, 0x2, 0x2, 0x75, 0x77, 0x5, 0x1c, 0xf, 
       0x2, 0x76, 0x6e, 0x3, 0x2, 0x2, 0x2, 0x76, 0x6f, 0x3, 0x2, 0x2, 0x2, 
       0x76, 0x70, 0x3, 0x2, 0x2, 0x2, 0x76, 0x71, 0x3, 0x2, 0x2, 0x2, 0x76, 
       0x72, 0x3, 0x2, 0x2, 0x2, 0x76, 0x73, 0x3, 0x2, 0x2, 0x2, 0x76, 0x74, 
       0x3, 0x2, 0x2, 0x2, 0x77, 0x1d, 0x3, 0x2, 0x2, 0x2, 0x78, 0x7d, 0x7, 
       0xb, 0x2, 0x2, 0x79, 0x7a, 0x7, 0x15, 0x2, 0x2, 0x7a, 0x7c, 0x7, 
       0xe, 0x2, 0x2, 0x7b, 0x79, 0x3, 0x2, 0x2, 0x2, 0x7c, 0x7f, 0x3, 0x2, 
       0x2, 0x2, 0x7d, 0x7b, 0x3, 0x2, 0x2, 0x2, 0x7d, 0x7e, 0x3, 0x2, 0x2, 
       0x2, 0x7e, 0x80, 0x3, 0x2, 0x2, 0x2, 0x7f, 0x7d, 0x3, 0x2, 0x2, 0x2, 
       0x80, 0x81, 0x7, 0x15, 0x2, 0x2, 0x81, 0x82, 0x7, 0xc, 0x2, 0x2, 
       0x82, 0x1f, 0x3, 0x2, 0x2, 0x2, 0x83, 0x86, 0x7, 0x15, 0x2, 0x2, 
       0x84, 0x86, 0x5, 0x26, 0x14, 0x2, 0x85, 0x83, 0x3, 0x2, 0x2, 0x2, 
       0x85, 0x84, 0x3, 0x2, 0x2, 0x2, 0x86, 0x21, 0x3, 0x2, 0x2, 0x2, 0x87, 
       0x88, 0x7, 0x15, 0x2, 0x2, 0x88, 0x23, 0x3, 0x2, 0x2, 0x2, 0x89, 
       0x8a, 0x7, 0x15, 0x2, 0x2, 0x8a, 0x25, 0x3, 0x2, 0x2, 0x2, 0x8b, 
       0x8c, 0x7, 0x6, 0x2, 0x2, 0x8c, 0x8d, 0x7, 0xb, 0x2, 0x2, 0x8d, 0x8e, 
       0x7, 0x15, 0x2, 0x2, 0x8e, 0x91, 0x7, 0xc, 0x2, 0x2, 0x8f, 0x91, 
       0x7, 0x16, 0x2, 0x2, 0x90, 0x8b, 0x3, 0x2, 0x2, 0x2, 0x90, 0x8f, 
       0x3, 0x2, 0x2, 0x2, 0x91, 0x27, 0x3, 0x2, 0x2, 0x2, 0x92, 0x98, 0x7, 
       0x18, 0x2, 0x2, 0x93, 0x98, 0x7, 0x15, 0x2, 0x2, 0x94, 0x98, 0x7, 
       0x17, 0x2, 0x2, 0x95, 0x96, 0x7, 0x12, 0x2, 0x2, 0x96, 0x98, 0x5, 
       0x28, 0x15, 0x2, 0x97, 0x92, 0x3, 0x2, 0x2, 0x2, 0x97, 0x93, 0x3, 
       0x2, 0x2, 0x2, 0x97, 0x94, 0x3, 0x2, 0x2, 0x2, 0x97, 0x95, 0x3, 0x2, 
       0x2, 0x2, 0x98, 0x29, 0x3, 0x2, 0x2, 0x2, 0x99, 0x9a, 0x7, 0x7, 0x2, 
       0x2, 0x9a, 0x9b, 0x7, 0xb, 0x2, 0x2, 0x9b, 0x9c, 0x7, 0x15, 0x2, 
       0x2, 0x9c, 0x9d, 0x7, 0xc, 0x2, 0x2, 0x9d, 0x2b, 0x3, 0x2, 0x2, 0x2, 
       0xf, 0x2f, 0x31, 0x3b, 0x46, 0x4f, 0x55, 0x61, 0x6a, 0x76, 0x7d, 
       0x85, 0x90, 0x97, 
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

qcisParser::Initializer qcisParser::_init;
