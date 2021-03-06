#pragma once

//
// Created by Anton Södergren on 2018-01-03.
//

#include <functional>
#include <vector>
#include <algorithm>
#include <cassert>


namespace observe
{
	// Observer with templates and function callbacks

	template<typename... Args>
	class subject;


	// Takes a function argument with Args arguments. Not meant to be inherited from.
	template<typename... Args>
	class observer
	{
	public:
		observer() = default;
		explicit observer(std::function<void(Args ...)> function);
		~observer();

		observer(const observer& other);
		observer& operator=(const observer& other);

		observer(observer&& other) noexcept;
		observer& operator=(observer&& other) noexcept;

		void set_function(std::function<void(Args ...)> function);

	private:
		void on_notify(Args ... args);
		void clear();

		std::function<void(Args ...)> function_{};
		std::vector<subject<Args...>*> subjects_{};

		friend class subject<Args...>;
	};


	// To be used together with Observer objects of the same template type.
	template<typename... Args>
	class subject
	{
	public:
		subject() = default;
		~subject();

		subject(const subject& other);
		subject& operator=(const subject& other);

		subject(subject&& other) noexcept;
		subject& operator=(subject&& other) noexcept;

		void add_observer(observer<Args...>& observer);
		void remove_observer(observer<Args...>& observer);
		void clear();
		void operator()(Args ... args);

	private:
		std::vector<observer<Args...>*> observers_{};
		int notify_counter_{};

		friend class observer<Args...>;
	};


	template<typename... Args>
	subject<Args...>& operator+=(subject<Args...>& lhs, observer<Args...>& rhs)
	{
		lhs.add_observer(rhs);
		return lhs;
	}


	template<typename... Args>
	subject<Args...>& operator-=(subject<Args...>& lhs, observer<Args...>& rhs)
	{
		lhs.remove_observer(rhs);
		return lhs;
	}


	template<typename... Args>
	observer<Args...>::observer(std::function<void(Args ...)> function)
		: function_{move(function)} { }


	template<typename ... Args>
	observer<Args...>::~observer()
	{
		clear();
	}


	template<typename ... Args>
	observer<Args...>::observer(const observer& other)
		: function_{other.function_} {}


	template<typename ... Args>
	observer<Args...>& observer<Args...>::operator=(const observer& other)
	{
		clear();
		function_ = other.function_;

		return *this;
	}


	template<typename ... Args>
	observer<Args...>::observer(observer&& other) noexcept
		: function_{move(other.function_)}, subjects_{move(other.subjects_)}
	{
		for (auto subject : subjects_)
		{
			auto it = std::find(subject->observers_.begin(), subject->observers_.end(), &other);
			assert(it != subject->observers_.end());
			*it = this;
		}
	}


	template<typename ... Args>
	observer<Args...>& observer<Args...>::operator=(observer&& other) noexcept
	{
		clear();
		function_ = move(other.function_);
		subjects_ = move(other.subjects_);
		for (auto subject : subjects_)
		{
			auto it = std::find(subject->observers_.begin(), subject->observers_.end(), &other);
			assert(it != subject->observers_.end());
			*it = this;
		}

		return *this;
	}


	template<typename ... Args>
	void observer<Args...>::set_function(std::function<void(Args ...)> function)
	{
		function_ = move(function);
	}


	template<typename ... Args>
	void observer<Args...>::on_notify(Args ... args)
	{
		function_(args...);
	}


	template<typename ... Args>
	void observer<Args...>::clear()
	{
		while(!subjects_.empty())
		{
			subjects_[subjects_.size() - 1]->remove_observer(*this);
		}
	}


	template<typename ... Args>
	subject<Args...>::~subject()
	{
		for(auto observer : observers_)
		{
			observer->subjects_.erase(
				std::remove(observer->subjects_.begin(), observer->subjects_.end(), this),
				observer->subjects_.end());
		}
	}


	template<typename ... Args>
	subject<Args...>::subject(const subject& other)
	{
		for(auto observer : other.observers_)
		{
			add_observer(*observer);
		}
	}


	template<typename ... Args>
	subject<Args...>& subject<Args...>::operator=(const subject& other)
	{
		clear();
		for(auto observer : other.observers_)
		{
			add_observer(*observer);
		}

		return *this;
	}


	template<typename ... Args>
	subject<Args...>::subject(subject&& other) noexcept : observers_{move(other.observers_)}
	{
		for(auto observer : observers_)
		{
			auto iter = std::find(observer->subjects_.begin(), observer->subjects_.end(), &other);
			assert(iter != observer->subjects_.end());
			*iter = this;
		}
	}


	template<typename ... Args>
	subject<Args...>& subject<Args...>::operator=(subject&& other) noexcept
	{
		clear();
		observers_ = move(other.observers_);
		for (auto observer : observers_)
		{
			auto iter = std::find(observer->subjects_.begin(), observer->subjects_.end(), &other);
			assert(iter != observer->subjects_.end());
			*iter = this;
		}

		return *this;
	}


	template<typename ... Args>
	void subject<Args...>::add_observer(observer<Args...>& observer)
	{
		observers_.push_back(&observer);
		observer.subjects_.push_back(this);
	}


	template<typename ... Args>
	void subject<Args...>::remove_observer(observer<Args...>& observer)
	{
		for(auto it = observer.subjects_.rbegin(); it != observer.subjects_.rend(); ++it)
		{
			if(*it == this)
			{
				observer.subjects_.erase(std::next(it).base());
				break;
			}
		}
		for (auto it = observers_.rbegin(); it != observers_.rend(); ++it)
		{
			if (*it == &observer)
			{
				if(notify_counter_ == 0)
				{
					observers_.erase(std::next(it).base());
				}
				else
				{
					*it = nullptr;
				}
				
				break;
			}
		}
	}


	template<typename ... Args>
	void subject<Args...>::clear()
	{
		for(auto observer : observers_)
		{
			observer->subjects_.erase(
				std::remove(observer->subjects_.begin(), observer->subjects_.end(), this),
				observer->subjects_.end());
		}
		if (notify_counter_ == 0)
		{
			observers_.clear();
		}
		else
		{
			for(auto it = observers_.begin(); it != observers_.end(); ++it)
			{
				*it = nullptr;
			}
		}
	}


	template<typename ... Args>
	void subject<Args...>::operator()(Args ... args)
	{
		++notify_counter_;
		auto size = observers_.size();
		for(size_t i = 0; i < size; ++i)
		{
			if(observers_[i])
			{
				observers_[i]->on_notify(args...);
			}
		}
		if(--notify_counter_ == 0)
		{
			observers_.erase(std::remove(observers_.begin(), observers_.end(), nullptr), observers_.end());
		}
	}
}
