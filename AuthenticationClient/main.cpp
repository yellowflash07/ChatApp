
#include <iostream>
#include "../authentication.pb.h"

int main()
{
	CreateAccountWeb createAccountWeb;
	createAccountWeb.set_requestid(123);
	createAccountWeb.set_email("ersgad@fas");
	createAccountWeb.set_plaintextpassword("password");

	std::string serialized;
	createAccountWeb.SerializeToString(&serialized);

	CreateAccountWeb deserialized;
	deserialized.ParseFromString(serialized);


	return 0;
}