
// Generated from qcis.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qcisParser.h"


namespace qcis {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by qcisParser.
 */
class  qcisVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by qcisParser.
   */
    virtual antlrcpp::Any visitProgram(qcisParser::ProgramContext *context) = 0;

    virtual antlrcpp::Any visitQuantumCircuitDeclaration(qcisParser::QuantumCircuitDeclarationContext *context) = 0;

    virtual antlrcpp::Any visitCircuitName(qcisParser::CircuitNameContext *context) = 0;

    virtual antlrcpp::Any visitQuantumStatement(qcisParser::QuantumStatementContext *context) = 0;

    virtual antlrcpp::Any visitMeasurementStatement(qcisParser::MeasurementStatementContext *context) = 0;

    virtual antlrcpp::Any visitQuantumGateCall(qcisParser::QuantumGateCallContext *context) = 0;

    virtual antlrcpp::Any visitQuantumBarrier(qcisParser::QuantumBarrierContext *context) = 0;

    virtual antlrcpp::Any visitQuantumGateName(qcisParser::QuantumGateNameContext *context) = 0;

    virtual antlrcpp::Any visitQuantumMeasurement(qcisParser::QuantumMeasurementContext *context) = 0;

    virtual antlrcpp::Any visitGateArgumentList(qcisParser::GateArgumentListContext *context) = 0;

    virtual antlrcpp::Any visitMeasurementArgumentList(qcisParser::MeasurementArgumentListContext *context) = 0;

    virtual antlrcpp::Any visitExpressionArgument(qcisParser::ExpressionArgumentContext *context) = 0;

    virtual antlrcpp::Any visitIntegerGateArgument(qcisParser::IntegerGateArgumentContext *context) = 0;

    virtual antlrcpp::Any visitExpression(qcisParser::ExpressionContext *context) = 0;

    virtual antlrcpp::Any visitExpressionTerminator(qcisParser::ExpressionTerminatorContext *context) = 0;

    virtual antlrcpp::Any visitListExpression(qcisParser::ListExpressionContext *context) = 0;

    virtual antlrcpp::Any visitIntegerMeasurementArgument(qcisParser::IntegerMeasurementArgumentContext *context) = 0;

    virtual antlrcpp::Any visitQubitMeasurementArgument(qcisParser::QubitMeasurementArgumentContext *context) = 0;

    virtual antlrcpp::Any visitQuantumCount(qcisParser::QuantumCountContext *context) = 0;

    virtual antlrcpp::Any visitClassicalCount(qcisParser::ClassicalCountContext *context) = 0;

    virtual antlrcpp::Any visitQubitReference(qcisParser::QubitReferenceContext *context) = 0;

    virtual antlrcpp::Any visitGateParameter(qcisParser::GateParameterContext *context) = 0;

    virtual antlrcpp::Any visitClassicalReference(qcisParser::ClassicalReferenceContext *context) = 0;


};

}  // namespace qcis
