#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdarg.h>

#ifndef MATHF_H_
struct vec2 {
	float x, y;
};

struct vec3 {
	float x, y, z;
};

struct vec4 {
	float x, y, z, w;
};
#endif

namespace saml
{

enum class VALUE_TYPE {
	NIL,
	NUMBER,
	STRING,
	TABLE,
	ARRAY,
};

struct Value
{
	VALUE_TYPE type;
	float number;
	std::string str;
	std::shared_ptr<std::map<std::string, Value>> table;
	std::shared_ptr<std::vector<Value>> array;

	Value() : type(VALUE_TYPE::NIL), number(0) {}
	Value(VALUE_TYPE type) : type(type), number(0)
	{
		if(type == VALUE_TYPE::TABLE)
		{
			table = std::make_shared<std::map<std::string, Value>>();
		}
		else if(type == VALUE_TYPE::ARRAY)
		{
			array = std::make_shared<std::vector<Value>>();
		}
	}

	Value(int value)
	{
		type = VALUE_TYPE::NUMBER;
		this->number = (float)value;
	}

	Value(float value)
	{
		type = VALUE_TYPE::NUMBER;
		this->number = value;
	}

	Value(const char *str)
	{
		type = VALUE_TYPE::STRING;
		this->str = std::string(str);
	}

	Value(const std::string &str)
	{
		type = VALUE_TYPE::STRING;
		this->str = str;
	}

	bool is_nil(){return type == VALUE_TYPE::NIL;}
	bool is_number(){return type == VALUE_TYPE::NUMBER;}
	bool is_string(){return type == VALUE_TYPE::STRING;}
	bool is_array(){return type == VALUE_TYPE::ARRAY;}
	bool is_table(){return type == VALUE_TYPE::TABLE;}

	int to_int(int default=0){return is_number() ? (int)number : default;}
	float to_float(float default=0){return is_number() ? number : default;}
	bool to_bool(bool default=false){
		if(is_number()) {
			return number >= 1.0f;
		} else if(is_string()) {
			if(str == "true")
				return true;
			if(str == "false")
				return false;
		}
		return default;
	}
	std::string to_string(std::string default=""){return is_string() ? str : default;}
	vec2 to_vec2(const vec2 &default=vec2()){return is_array() ? vec2(at(0).to_float(), at(1).to_float()) : default;}
	vec3 to_vec3(const vec3 &default=vec3()){return is_array() ? vec3(at(0).to_float(), at(1).to_float(), at(2).to_float()) : default;}
	vec4 to_vec4(const vec4 &default=vec4()){return is_array() ? vec4(at(0).to_float(), at(1).to_float(), at(2).to_float(), at(3).to_float()) : default;}

	int get_size()
	{
		if(type == VALUE_TYPE::ARRAY)
		{
			return (int)array->size();
		}
		else if(type == VALUE_TYPE::TABLE)
		{
			return (int)table->size();
		}

		return 0;
	}


	///////////////////////////////////////////////////////////////////////////
	// array method
	void add(const Value &value)
	{
		if(type != VALUE_TYPE::ARRAY) return;
		array->push_back(value);
	}

	void add(int value)
	{
		add(Value(value));
	}

	void add(float value)
	{
		add(Value(value));
	}

	void add(const std::string &str)
	{
		add(Value(str));
	}


	///////////////////////////////////////////////////////////////////////////
	// table method
	void set_value(std::string key, const Value &value) {
		if(type != VALUE_TYPE::TABLE) return;
		table->insert(std::pair<std::string, Value>(key, value));
	}

	Value get_value(std::string key)
	{
		if(type != VALUE_TYPE::TABLE) return Value();
		if(table->count(key) == 0) return Value();
		return table->find(key)->second;
	}

	int get_int(std::string key)
	{
		Value v = get_value(key);
		return v.type == VALUE_TYPE::NUMBER ? (int)v.number : 0;
	}

	float get_float(std::string key)
	{
		Value v = get_value(key);
		return v.type == VALUE_TYPE::NUMBER ? v.number : 0;
	}

	std::string get_string(std::string key)
	{
		Value v = get_value(key);
		return v.type == VALUE_TYPE::STRING ? v.str : "";
	}

	std::shared_ptr<std::map<std::string, Value>> get_table(std::string key)
	{
		Value v = get_value(key);
		return v.type == VALUE_TYPE::TABLE ? v.table : nullptr;
	}

	std::shared_ptr<std::vector<Value>> get_array(std::string key)
	{
		Value v = get_value(key);
		return v.type == VALUE_TYPE::ARRAY ? v.array : nullptr;
	}

	Value operator[] (const char *key)
	{
		return get_value(key);
	}

	Value operator[] (int index)
	{
		return at(index);
	}

	Value at(int index)
	{
		if(index < 0) return Value();
		if(index >= get_size()) return Value();

		if(type == VALUE_TYPE::ARRAY)
		{
			return array->at(index);
		}
		else if(type == VALUE_TYPE::TABLE)
		{
			auto it = table->begin();
			std::advance(it, index);
			return it->second;
		}

		return Value();
	}
};

Value parse(const char *str);
Value parse_file(const char *filename);




#ifdef SAML_IMPLEMENTATION

enum TokenType{
	TOKEN_ERROR,
	TOKEN_NUMBER,
	TOKEN_STRING,
	TOKEN_IDENTIFIER,
	TOKEN_TAB,
	TOKEN_EXTEND,
	TOKEN_LINEEND,
	TOKEN_EQUAL,
	TOKEN_RESERVED,
	TOKEN_EOF,
};

struct Token {
	TokenType type;		// type of token
	const char *start;	// token string
	int len;			// length of token
	int line;

	inline bool compare_string(const char *str)
	{
		return strlen(str) == len && strncmp(start, str, len) == 0;
	}

	inline std::string to_string()
	{
		return std::string(start, len);
	}
};

struct Scanner {
	const char *start;
	const char *current;
	int line;

	void init(const char *str) {
		start = str;
		current = str;
		line = 1;
	}

	static inline int is_alpha(char c)
	{
		return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' || c == '.' || c== '/';
	}

	static inline int is_digit(char c)
	{
		return (('0' <= c && c <= '9') || '-' == c || '.' == c);
	}

	bool is_at_end() {
		return *current == '\0';
	}

	char peek() {
		return *current;
	}

	char peek_next() {
		if(is_at_end()) return '\0';
		return current[1];
	}

	char scan_advance() {
		current++;
		return current[-1];
	}

	void skip_whitespace()
	{
		for(;;)
		{
			char c = peek();
			switch(c)
			{
			case ' ':
			case '\t':
			case '\r':
				scan_advance();
				break;

			case '/':
				if(peek_next() == '/')
				{
					// skip to the end of the line
					while(peek() != '\n' && !is_at_end()) scan_advance();
				}
				else
				{
					return;
				}
				break;

			case '#':
				// skip to the end of the line
				while(peek() != '\n' && !is_at_end()) scan_advance();
				break;

			default:
				return;
			}
		}
	}


	Token make_token(TokenType type)
	{
		Token token;
		token.type = type;
		token.start = start;
		token.len = (int)(current - start);
		token.line = line;
		return token;
	}

	Token error_token(const char *message)
	{
		Token token;
		token.type = TOKEN_ERROR;
		token.start = message;
		token.len = (int)strlen(message);
		token.line = line;
		return token;
	}

	Token make_token_identifier()
	{
		while(is_alpha(peek()) || is_digit(peek())) scan_advance();
		return make_token(TOKEN_IDENTIFIER);
	}

	Token make_token_number()
	{
		while(is_digit(peek())) scan_advance();
		if(peek() == '.' && is_digit(peek_next()))
		{
			// consume the "."
			scan_advance();
			while(is_digit(peek())) scan_advance();
		}
		return make_token(TOKEN_NUMBER);
	}

	Token make_token_string()
	{
		while(peek() != '"' && !is_at_end())
		{
			if(peek() == '\n') line++;
			scan_advance();
		}

		if(is_at_end()) return error_token("Unterminated string.");
		scan_advance();	// The closing quote
		return make_token(TOKEN_STRING);
	}

	Token scan_token() {
		skip_whitespace();
		start = current;
		if(is_at_end()) return make_token(TOKEN_EOF);
		char c = scan_advance();

		if(is_alpha(c)) return make_token_identifier();
		if(is_digit(c)) return make_token_number();

		switch(c)
		{
		case '=': return make_token(TOKEN_EQUAL);
		case '"': return make_token_string();
		case '\n': {line++; return scan_token();}
				 //case '\t': return make_token(TOKEN_TAB);
		case ':': return make_token(TOKEN_EXTEND);
		case '[': return make_token(TOKEN_RESERVED);
		case ']': return make_token(TOKEN_RESERVED);
		case '{': return make_token(TOKEN_RESERVED);
		case '}': return make_token(TOKEN_RESERVED);
		case ',': return make_token(TOKEN_RESERVED);
		}

		return error_token("Unexpected character.");
	}

	Token peek_token()
	{
		Scanner last_scanner = *this;
		Token token = scan_token();
		*this = last_scanner;
		return token;
	}

	Token peek_next_token()
	{
		Scanner last_scanner = *this;
		scan_token();
		Token token = scan_token();
		*this = last_scanner;
		return token;
	}
};
static Scanner _scanner;

static void error_at(int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	//int pos = (int)(loc - src);
	char str[512] = {0};
	vsprintf(str, fmt, ap);
	printf("line:%d %s\n", line, str);
	system("pause");
	va_end(ap);
	//exit(1);
}

static Value parse_block();
static Value parse_as_table()
{
	Value table = Value(VALUE_TYPE::TABLE);
	while(1)
	{
		Token key = _scanner.scan_token();
		if(key.type != TOKEN_IDENTIFIER)
		{
			error_at(_scanner.line, "expected a identifier\n");
			break;
		}

		if(!_scanner.scan_token().compare_string("="))
		{
			error_at(_scanner.line, "expected \"=\"\n");
			break;
		}

		Token value_token = _scanner.scan_token();
		if(value_token.type == TOKEN_NUMBER)
		{
			table.set_value(key.to_string(), strtof(value_token.start, nullptr));
		} 
		else if(value_token.type == TOKEN_STRING)
		{
			table.set_value(key.to_string(), std::string(value_token.start+1, value_token.len-2));
		}
		else if(value_token.compare_string("{"))
		{
			// parse block
			Value block = parse_block();
			table.set_value(key.to_string(), block);
		}


		if(_scanner.peek() == '\r' || _scanner.peek() == '\n')
		{
			if(_scanner.peek_token().compare_string("}")){_scanner.scan_token(); break;}
			continue;
		}

		Token token = _scanner.peek_token();
		if(token.compare_string(","))	// next item
		{
			_scanner.scan_token();
			if(_scanner.peek_token().compare_string("}")){_scanner.scan_token(); break;} // ignore final comma
			continue;
		}
		else if(token.compare_string("}")) // close table
		{
			_scanner.scan_token();
			break;
		}
		else
		{
			error_at(_scanner.line, "expected \",\" or \"}\"\n");
			break;
		}
	}
	return table;
}

static Value parse_as_array()
{
	Value array = Value(VALUE_TYPE::ARRAY);
	while(1)
	{
		Token val_token = _scanner.scan_token();
		if(val_token.type == TOKEN_NUMBER)
		{
			array.add(Value(strtof(val_token.start, nullptr)));
		} 
		else if(val_token.type == TOKEN_STRING)
		{
			array.add(Value(std::string(val_token.start+1, val_token.len-2)));
		}
		else if(val_token.compare_string("{"))
		{
			Value block = parse_block();
			array.add(block);
		}
		else
		{
			error_at(_scanner.line, "expected a value");
			break;
		}


		if(_scanner.peek() == '\r' || _scanner.peek() == '\n')
		{
			if(_scanner.peek_token().compare_string("}")){_scanner.scan_token(); break;}
			continue;
		}

		Token token = _scanner.peek_token();
		if(token.compare_string(","))
		{
			_scanner.scan_token();
			if(_scanner.peek_token().compare_string("}")){_scanner.scan_token(); break;} // ignore final comma
			continue;
		}
		else if(token.compare_string("}")) // close array
		{
			_scanner.scan_token();
			break;
		}
		else
		{
			error_at(_scanner.line, "expected \",\" or \"}\"");
			break;
		}
	}
	return array;
}

static Value parse_block()
{
	Token val_token = _scanner.peek_token();
	Value val;
	if(val_token.type == TOKEN_IDENTIFIER)
	{
		val = parse_as_table();
	}
	else if(val_token.type == TOKEN_NUMBER || val_token.type == TOKEN_STRING || val_token.compare_string("{"))
	{
		val = parse_as_array();
	}
	else if(val_token.compare_string("}"))
	{
		return val;
	}
	return val;
}


Value parse(const char *str) {
	// init scanner
	_scanner.init(str);

	Value root = Value(VALUE_TYPE::TABLE);
	Value cur_table = root;

	Token t = _scanner.scan_token();
	while(t.type != TOKEN_EOF) {
		
		// Assing the field
		if(t.type == TOKEN_IDENTIFIER)
		{
			std::string key(t.start, t.len);

			// expect next token is "="
			t = _scanner.scan_token();
			if(t.type != TOKEN_EQUAL) {
				error_at(_scanner.line, "expected \"=\"");
				break;
			}

			// value token
			Token value_token = _scanner.scan_token();
			if(value_token.type == TOKEN_NUMBER)
			{
				cur_table.set_value(key, strtof(value_token.start, nullptr));
			} 
			else if(value_token.type == TOKEN_STRING)
			{
				cur_table.set_value(key, std::string(value_token.start+1, value_token.len-2));
			}
			else if(*value_token.start == '{')
			{
				Value block = parse_block();
				if(block.type != VALUE_TYPE::NIL)
				{
					cur_table.set_value(key, block);
				}
			}
		}

		t = _scanner.scan_token();
	}

	return root;
}

Value parse_file(const char *filename)
{
	Value value;
	FILE *fp = fopen(filename, "rb");
	if(fp == nullptr)
		return value;

	// Load text
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	char *str = new char[size+1];
	fseek(fp, 0, SEEK_SET);
	fread(str, sizeof(char), size, fp);
	str[size] = '\0';
	value = saml::parse(str);

	delete[] str;
	fclose(fp);

	return value;
}


#endif

}