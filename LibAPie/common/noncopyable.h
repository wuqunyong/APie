#pragma once

class Noncopyable
{
protected:
	Noncopyable()
	{
	}
	~Noncopyable()
	{
	}
private:
	// emphasize the following members are private
	Noncopyable(const Noncopyable&);
	const Noncopyable& operator=(const Noncopyable&);
};

