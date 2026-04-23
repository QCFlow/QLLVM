
// Generated from qcis.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"


namespace qcis {


class  qcisLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, SEMICOLON = 6, LPAREN = 7, 
    RPAREN = 8, LBRACKET = 9, RBRACKET = 10, COLON = 11, COMMA = 12, DOT = 13, 
    EQUALS = 14, PLUS = 15, MINUS = 16, STAR = 17, SLASH = 18, Integer = 19, 
    QubitId = 20, Constant = 21, RealNumber = 22, StringLiteral = 23, Identifier = 24, 
    Whitespace = 25, LineComment = 26
  };

  explicit qcisLexer(antlr4::CharStream *input);
  ~qcisLexer();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

}  // namespace qcis
