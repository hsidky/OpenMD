 /*
 * Copyright (c) 2005 The University of Notre Dame. All Rights Reserved.
 *
 * The University of Notre Dame grants you ("Licensee") a
 * non-exclusive, royalty free, license to use, modify and
 * redistribute this software in source and binary code form, provided
 * that the following conditions are met:
 *
 * 1. Acknowledgement of the program authors must be made in any
 *    publication of scientific results based in part on use of the
 *    program.  An acceptable form of acknowledgement is citation of
 *    the article in which the program was described (Matthew
 *    A. Meineke, Charles F. Vardeman II, Teng Lin, Christopher
 *    J. Fennell and J. Daniel Gezelter, "OOPSE: An Object-Oriented
 *    Parallel Simulation Engine for Molecular Dynamics,"
 *    J. Comput. Chem. 26, pp. 252-271 (2005))
 *
 * 2. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 3. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 * This software is provided "AS IS," without a warranty of any
 * kind. All express or implied conditions, representations and
 * warranties, including any implied warranty of merchantability,
 * fitness for a particular purpose or non-infringement, are hereby
 * excluded.  The University of Notre Dame and its licensors shall not
 * be liable for any damages suffered by licensee as a result of
 * using, modifying or distributing the software or its
 * derivatives. In no event will the University of Notre Dame or its
 * licensors be liable for any lost revenue, profit or data, or for
 * direct, indirect, special, consequential, incidental or punitive
 * damages, however caused and regardless of the theory of liability,
 * arising out of the use of or inability to use software, even if the
 * University of Notre Dame has been advised of the possibility of
 * such damages.
 */

#include "selection/TokenMap.hpp"

namespace oopse {

TokenMap* TokenMap::instance_ = NULL;

TokenMap::TokenMap() {
    tokenMap_.insert(std::make_pair("define", Token(Token::define, std::string("define"))));
    tokenMap_.insert(std::make_pair("select", Token(Token::select, std::string("select")))); 
    //expressions
    tokenMap_.insert(std::make_pair("(", Token(Token::leftparen, std::string("("))));
    tokenMap_.insert(std::make_pair(")", Token(Token::rightparen, std::string(")"))));
    tokenMap_.insert(std::make_pair("-", Token(Token::hyphen, std::string("-"))));
    tokenMap_.insert(std::make_pair("and", Token(Token::opAnd, std::string("and"))));
    tokenMap_.insert(std::make_pair("or", Token(Token::opOr, std::string("or"))));
    tokenMap_.insert(std::make_pair("not", Token(Token::opNot, std::string("not"))));
    tokenMap_.insert(std::make_pair("<", Token(Token::opLT, std::string("<"))));
    tokenMap_.insert(std::make_pair("<=", Token(Token::opLE, std::string("<="))));
    tokenMap_.insert(std::make_pair(">=", Token(Token::opGE, std::string(">="))));
    tokenMap_.insert(std::make_pair(">", Token(Token::opGT, std::string(">="))));
    tokenMap_.insert(std::make_pair("==", Token(Token::opEQ, std::string("=="))));
    tokenMap_.insert(std::make_pair("!=", Token(Token::opNE, std::string("!="))));
    tokenMap_.insert(std::make_pair("within", Token(Token::within, std::string("within"))));
    tokenMap_.insert(std::make_pair(".", Token(Token::dot, std::string("."))));
    tokenMap_.insert(std::make_pair("mass", Token(Token::mass, std::string("mass"))));
    tokenMap_.insert(std::make_pair("dipole", Token(Token::dipole, std::string("dipole"))));
    tokenMap_.insert(std::make_pair("charge", Token(Token::charge, std::string("charge"))));
    tokenMap_.insert(std::make_pair("name", Token(Token::name, std::string("name"))));
    tokenMap_.insert(std::make_pair("index", Token(Token::index, std::string("index"))));
    tokenMap_.insert(std::make_pair("molname", Token(Token::molname, std::string("molname"))));
    tokenMap_.insert(std::make_pair("molindex", Token(Token::molindex, std::string("molindex"))));

    tokenMap_.insert(std::make_pair("*", Token(Token::asterisk, std::string("*"))));
    tokenMap_.insert(std::make_pair("all", Token(Token::all, std::string("all"))));
    tokenMap_.insert(std::make_pair("none", Token(Token::none, std::string("none"))));
}

Token* TokenMap::getToken(const std::string& ident) {
    std::map<std::string, Token>::iterator i = tokenMap_.find(ident);

    return i != tokenMap_.end() ? &(i->second) : NULL;
}
}
