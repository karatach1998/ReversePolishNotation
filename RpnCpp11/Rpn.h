#ifndef RPN_H
#define RPN_H


#include <vector>
#include <memory>
#include <stack>


// Какой-либо ТОКЕН.
struct Token
{
    const bool isOperation; // Является ли токен операцией.

protected:
    // Конструктор находится в защищенном разделе,
    // так как класс Token сам по себе ничего не значит.
    Token( bool val ) : isOperation(val) { }

    virtual ~Token() {}
};



// Промежуточный класс Operand,
// реализующий полиморфное поведение дочерних классов.
template <typename T>
class Operand : public Token
{
public:
    // Метод get() делает данный класс абстрактным.
    virtual T& get() = 0;

protected:
    Operand() : Token(false) { }
};



// Какой-либо (ТОКЕН->)ОПЕРАНД-ПЕРЕМЕННАЯ.
template <typename T>
class OperandVariable : public Operand<T>
{
private:
    const char* const name_; // Стока, хранящая Имя переменной.
    T var_; // Область памяти, которая и будет непосредственно изменяться.

public:
    // Конструктор принимает только имя переменной.
    explicit OperandVariable( const char* name ) : name_(name){ }

    const char* getName() const { return name_; }

    // Возвращает ссылку на переменную.
    virtual T& get() override { return var_; }
};



// Какой-либо (ТОКЕН->)ОПЕРЕАНД-ЗНАЧЕНИЕ.
template <typename T>
class OperandLiteral : public Operand<T>
{
private:
    T value_; // Значение.

public:
    // Конструктор принимает литеральное значение.
    explicit OperandLiteral( T val ) : value_(val) { }

    // Вовращает ссылку на значение.
    virtual T& get() override { return value_; }
};



// Какая-либо (ТОКЕН->)ОПЕРАЦИЯ.
template <typename T>
class Operation : public Token
{
public:
    typedef T (*TF1)( T );
    typedef T (*TF2)( T, T );


    // Конструктор принимает указательна функцию,
    // соответсвующую данной операции, и количество её аргументов.
    explicit Operation( void* f, unsigned char n ) : nOperands_(n), f1((TF1)f), Token(true) { }

    unsigned char nOperand() const { return nOperands_; }

private:
    const unsigned char nOperands_;

public:
    const union
    {
        TF1 f1; // указатель на функцию с одним аргументом
        TF2 f2; // указатель на функцию с двумя агументами
    };
};



template <typename T>
class OperationExtended : public Operation<T>
{
public:
    // Костроуктор принимает 'имя' функции, её приоритет,
    // адрес функции её задающей и количество аргументов.
    explicit OperationExtended( const char* name,
                                unsigned char prioritet,
                                void* f,
                                unsigned char n )
        : name_(name), prioritet_(prioritet), Operation<T>(f, n) { }


    const unsigned char getPrioritet() const
    { return prioritet_; }


    const char* getName() const
    { return name_; }

private:
    const char* const name_;
    const unsigned char prioritet_;
};



template <typename T>
class Rpn
{
public:
    typedef typename std::vector<OperationExtended<T>> Operations;
    typedef typename std::shared_ptr<Token> PToken;
    typedef typename std::vector<PToken> TExp;
    typedef struct
    {
        TExp exp;
        std::vector<bool*> vVar;
        std::size_t varN;
    } TDExp;


    // Конструктор принимает ссылку на std::vector операций,
    // адрес функции определяющей, является ли данная подстока
    // допустимым представлением операнда,
    // адрес функции преобразовывающей подстроку в значение типа T.
    Rpn( const Operations& operations, std::size_t (* f1)( const char*, bool& ),
         T (* f2)( const char* ))
            : operations_(operations), isOperand(f1), strToValue(f2) { }


    // Преобразовывае входную строку в ОПЗ и возвращает
    // std::vector Операндов или Операций.
    TExp convert( const char* sourceStr ) const;

    // Аналогично convert. Воозвращает структуру, хранящую помимо самого
    // вектора токенов, еще и вектор адресов переменных и их количество.
    TDExp convertWithVar( const char* sourceStr ) const;

private:
    std::size_t isOperation( const char* str, unsigned& code ) const;
    std::size_t (* isOperand)( const char*, bool& isVariable );
    T (*strToValue)( const char* );

private:
    Operations operations_;
};


template <typename T>
typename Rpn<T>::TExp Rpn<T>::convert( const char* sourceStr ) const
{
    // Устанавливает указатель по адресу ps на первый значащий символ
    // строки *ps.
    void getNextWord( const char** ps );

    // Возвращает некоторое закодированное представление имени переменной.
    unsigned hashFunction( const char* );


    Rpn<T>::TExp result;
    std::stack<unsigned> operationStack;
    struct data
    {
        PToken pop;
        unsigned code;
    };
    std::vector<data> hashCodesOfVariable;
    std::size_t offset;
    unsigned cOperationCode;
    bool isVariable;
    unsigned maxCode = operations_.size();


    while (*sourceStr) {
        getNextWord(&sourceStr);
        if (offset = isOperation(sourceStr, cOperationCode)) {
            sourceStr += offset;
            while (!operationStack.empty() && operationStack.top() < maxCode
                   && operations_[operationStack.top()].getPrioritet() <=
                      operations_[cOperationCode].getPrioritet()) {
                // добавляем элемент выходной вектор.
                result.push_back(PToken(
                        new Operation<T>(operations_[operationStack.top()])));
                operationStack.pop();
            }
            operationStack.push(cOperationCode);
        } else if (offset = isOperand(sourceStr, isVariable)) {
            char* tmpS = const_cast<char*>(sourceStr);
            char tmpC = tmpS[offset];
            tmpS[offset] = '\0';
            if (isVariable) {
                unsigned hashCode = hashFunction(tmpS);
                std::size_t j;
                for (j = 0; j < hashCodesOfVariable.size(); ++j)
                    if (hashCode == hashCodesOfVariable[j].code)
                        break;

                if (j == hashCodesOfVariable.size())
                    hashCodesOfVariable
                            .push_back({PToken(new OperandVariable<T>(tmpS)),
                                        hashCode});
                result.push_back(hashCodesOfVariable[j].pop);
            }
            else
                result.push_back(PToken(new OperandLiteral<T>(strToValue(tmpS))));
            tmpS[offset] = tmpC;
            sourceStr += offset;

        } else if (*sourceStr == '(') {
            operationStack.push(maxCode);
            ++sourceStr;
        } else if (*sourceStr == ')') {
            ++sourceStr;
            while (!operationStack.empty() && operationStack.top() < maxCode)
            {
                result.push_back(PToken(
                        new Operation<T>(operations_[operationStack.top()])));
                operationStack.pop();
            }
            if (!operationStack.empty()) // == 0
                operationStack.pop();
            else {
                // Если стек пуст, а открывающая скобка не встретилась,
                // это означает, что где-то не была открыта скобка.
                throw ')';
            }
        } else
            throw *sourceStr;
    }

    while (!operationStack.empty()) {
        if (operationStack.top() < maxCode) {
            result.push_back(PToken(
                    new Operation<T>(operations_[operationStack.top()])));
            operationStack.pop();
        } else
            throw '(';
    }

    return result;
}



template <typename T>
typename Rpn<T>::TDExp
Rpn<T>::convertWithVar( const char* sourceStr ) const
{
    void getNextWord( const char** );
    unsigned hashFunction( const char* );

    Rpn<T>::TExp result;
    std::stack<unsigned> operationStack;
    struct data
    {
        PToken pop;
        unsigned code;
    };
    std::vector<data> hashCodesOfVariable;
    std::size_t offset;
    unsigned cOperationCode;
    bool isVariable;
    unsigned maxCode = operations_.size();


    // Пока не конец строки.
    while (*sourceStr) {
        // Проускаем все незначащие символы.
        getNextWord(&sourceStr);
        // Если подсторка является знаком операции,
        if (offset = isOperation(sourceStr, cOperationCode)) {
            // то помещаем этот элемент в стек.
            sourceStr += offset;

            // Пока стек не пуст, и это не открывающаяся скобка,
            // и приоритет операции на вершине стека выще,
            // чем приоритет текущей операции,
            while (!operationStack.empty() && operationStack.top() < maxCode
                   && operations_[operationStack.top()].getPrioritet() <=
                      operations_[cOperationCode].getPrioritet()) {
                // добавляем элемент выходной вектор.
                result.push_back(PToken(
                        new Operation<T>(operations_[operationStack.top()])));
                operationStack.pop();
            }
            // Кладем код текущей операции в стек.
            operationStack.push(cOperationCode);
        } else if (offset = isOperand(sourceStr, isVariable)) {
            // Если текущая подстрока является операндом,
            char* tmpS = const_cast<char*>(sourceStr);
            char tmpC = tmpS[offset];
            tmpS[offset] = '\0';
            // и если это -- переменная, то:
            if (isVariable) {
                // 1) вычисляем специальный код этого операнда,
                //    исходя из его стокового представления;
                unsigned hashCode = hashFunction(tmpS);
                std::size_t j;
                // 2) ищим вычисленный код среди уже имеющихся;
                for (j = 0; j < hashCodesOfVariable.size(); ++j)
                    if (hashCode == hashCodesOfVariable[j].code)
                        break;

                // 3) если таков не найден,
                if (j == hashCodesOfVariable.size())
                    // то добавим текущий операнд переменную в стек;
                    hashCodesOfVariable
                            .push_back({PToken(new OperandVariable<T>(tmpS)),
                                        hashCode});
                // 4) добавляем соответствующий std::shared_ptr
                //    в выходной вектор result.
                result.push_back(hashCodesOfVariable[j].pop);
            }
            else
                // иначе ели это литерал, то вычислим его значение,
                // записанное в данной подстроке,
                // и запишем его в выходной result.
                result.push_back(PToken(new OperandLiteral<T>(strToValue(tmpS))));
            tmpS[offset] = tmpC;
            sourceStr += offset;

        } else if (*sourceStr == '(') {
            // Если текущий символ -- открывающаяся скобка,
            // то добавим maxCode в стек для маркировки данного символа.
            operationStack.push(maxCode);
            ++sourceStr;
        } else if (*sourceStr == ')') {
            // Если это -- закрывающаяся скобка,
            ++sourceStr;
            // то пока стек не пуст и не наткнулись на ')',
            while (!operationStack.empty() && operationStack.top() < maxCode)
            {
                // добавляем операции в выходной вектор result.
                result.push_back(PToken(
                        new Operation<T>(operations_[operationStack.top()])));
                operationStack.pop();
            }

            if (!operationStack.empty()) // == 0
                operationStack.pop();
            else {
                // Если стек пуст, а открывающая скобка не встретилась,
                // это означает, что где-то не была открыта скобка.
                throw ')';
            }
        } else
            throw *sourceStr;
    }

    // Пока стек не пуст, добавляем операцию в выходной вектор result.
    while (!operationStack.empty()) {
        if (operationStack.top() < maxCode) {
            result.push_back(PToken(
                    new Operation<T>(operations_[operationStack.top()])));
            operationStack.pop();
        } else
            throw '(';
    }

    std::vector<bool*> vp(hashCodesOfVariable.size());
    for (std::size_t i = 0; i < vp.size(); ++i)
        vp[i] = &(static_cast<Operand<T>*>(hashCodesOfVariable[i].pop.get())->get());

    return {std::move(result), std::move(vp), hashCodesOfVariable.size()};
}



template <typename T>
std::size_t Rpn<T>::isOperation( const char* str, unsigned int& code ) const
{
    std::size_t n = operations_.size();
    std::size_t j;


    for (std::size_t i = 0; i < n; ++i) {
        // сравниваем подстроку со знаками операций.
        for (j = 0; str[j] == operations_[i].getName()[j] && str[j]; ++j)
            ;

        // Если операция найдена,
        if (!operations_[i].getName()[j]) {
            // то сохраняем её код и возвращаем смещение.
            code = i;
            return j;
        }
    }

    return 0;
}


#endif