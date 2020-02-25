#pragma once


#include <exception>
#include <stdexcept>
#include <atomic>

template<class T>
class Expected
{
protected:
    union
    {
        T result;
        std::exception_ptr spam;
    };

    bool gotResult;
    Expected() {};

public:

    /*constructors*/
    Expected(const T& r) : result(r), gotResult(true){}
    Expected(T&& r) : result(std::move(r)), gotResult(true) {}
    Expected(const Expected& e) : gotResult(e.gotResult)
    {
        if (gotResult)
            new(&result) T(e.result);
        else
            new(&spam) std::exception_ptr(e.spam);
    }
    Expected(Expected&& e) : gotResult(e.gotResult)
    {
        if (gotResult)
            new(&result) T(std::move(e.result));
        else
            new(&spam) std::exception_ptr(std::move(e.spam));
    }
    ~Expected(){}

    /*swap two Expected*/
    void swap(Expected& e)
    {
        if (gotResult)
        {
            if (e.gotResult)
                std::swap(result, e.result);
            else
            {
                auto t = std::move(e.spam);
                new(&e.result) T(std::move(result));
                new(&spam) std::exception_ptr(t);
                std::swap(gotResult, e.gotResult);
            }
        }
        else
        {
            if (e.gotResult)
                e.swap(*this);
            else
                std::swap(spam, e.spam);
        }
    }

    /*Create Expected from exception*/
    template<typename E>
    Expected<T>(E const& e) : spam(std::make_exception_ptr(e)), gotResult(false){}

    template<class E>
    static Expected<T> fromException(const E& exception)
    {
        if (typeid(exception) != typeid(E))
            throw std::invalid_argument("slicing detected!\n");
        return fromException(std::make_exception_ptr(exception));
    }

    static Expected<T> fromException(std::exception_ptr p)
    {
        Expected<T> e;
        e.gotResult = false;
        new(&e.spam) std::exception_ptr(std::move(p));
        return e;
    }

    static Expected<T> fromException()
    {
        return fromException(std::current_exception());
    }

    /*operator overload*/
    Expected<T>& operator=(const Expected<T>& e)
    {
        gotResult = e.gotResult;
        if (gotResult)
            new(&result) T(e.result);
        else
            new(&spam) std::exception_ptr(e.spam);
        return *this;
    }

    /*getter*/
    bool isValid() const { return gotResult; };

    T& get()
    {
        if (!gotResult)
            std::rethrow_exception(spam);
        return result;
    }

    const T& get() const
    {
        if (!gotResult)
            std::rethrow_exception(spam);
        return result;
    }

    /*test for exception*/
    template<class E>
    bool hasException() const
    {
        try
        {
            if (!gotResult)
                std::rethrow_exception(spam);
        }
        catch (const E & obj)
        {
            (void)obj;
            return true;
        }
        catch (...)
        {

        }
        return false;
    }


    friend class Expected<void>;
};

template<>
class Expected<void>
{
    std::exception_ptr spam;

public:
    /*constructors*/
    template<typename E>
    Expected(E const& e) : spam(std::make_exception_ptr(e)){}
    template<typename T>
    Expected(const Expected<T>& e)
    {
        if (e.gotResult)
            new(&spam) std::exception_ptr(e.spam);
    }

    Expected(Expected&& o) : spam(std::move(o.spam)){}
    Expected() : spam(){}

    /*operator overload*/
    Expected& operator=(const Expected& e)
    {
        if (!e.isValid())
            this->spam = e.spam;
        return *this;
    }

    bool isValid() const { return !spam; }
    void get() const { if (!isValid()) std::rethrow_exception(spam); }
    void suppress(){}
};