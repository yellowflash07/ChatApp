#pragma once

#include "../authentication.pb.h"

class AuthenticationClient
{
	public:
	AuthenticationClient();
	~AuthenticationClient();

	void CreateAccount(const std::string& email, const std::string& password);
	void Login(const std::string& email, const std::string& password);
};

