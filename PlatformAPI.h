/*
 *  Authentication Platform API
 *  Author: Manuel Rojas
 */

#pragma once
#include <iostream>
#include <queue>
#include <fstream>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <vector>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/bind.hpp>
#include "tinyxml.h"


namespace PLAT
		{
		  using namespace std;

		  typedef unsigned __int32 Version;

		  ///typedef uint32_t Version;

		  // api version
		  const Version THIS_VERSION = 1;

		  // result code.
		  //    >0 -> success with warning/info
		  //    0  -> success
		  //    <0 -> error

		  typedef __int32 Result;
		  ///typedef int32_t Result;

		  // api result codes

		  #define RC_AUTH_OK 0
		  #define RC_AUTH_ERROR -1
		  #define RC_AUTH_BAD_VERSION -2
		  #define RC_AUTH_BAD_UNAME_PASSWD -3
		  #define RC_AUTH_BANNED_USER -4
		  #define RC_AUTH_USER_NOTIN_GROUP -5
		  #define RC_AUTH_USER_NOTIN_GMSKU -6
		  #define RC_AUTH_NO_RESPONSE -7


		  boost::mutex RequestQueueResource;
		  boost::mutex ResponseQueueResource;
		  boost::mutex io_mutex;

		  
		  
		  int gcertCount = 0;    
		  int PreviousCertCount = 0;
		  int usercounter = 0;
		  
			// helper func
		  inline bool isok(Result rc) { return (rc>=RC_AUTH_OK); }

		  class PlatAPI_Impl;

		  class PlatAPI
						{ 
							public:
							PlatAPI();
							~PlatAPI();

							// account id
							typedef unsigned __int32 AccountID;
							///typedef uint32_t AccountID;

							// user
							class User_Impl;

							class User
									{
										public:
								    User();
								    User(wstring, wstring, wstring);

								    AccountID             account() const;

								    wstring               setName (wstring);
								    wstring               name() const;

								    wstring               setPassword (wstring);
								    wstring               password() const;

								    wstring               setGamesku(wstring);
								    wstring               gamesku() const;

								    Result                setReturnCode (Result);
								    Result                returnCode() const;

								    const vector<wstring> permissions() const;
								
										private:
										User_Impl *_impl;
									};
		      
						  // authentication

						  User   *getAuthCertificates( Result &rc );
						  Result  authenticateUser( const wstring &user, const wstring &password, const wstring & GameSKU );
						  Result  getAuthCerts(User* users, int* certCount);
						  void PumpFunction ();


						  private:
						  PlatAPI_Impl *_impl;
		  			};
		   
		  list <PlatAPI::User> RequestQueue; 
		  list <PlatAPI::User> ResponseQueue; 
		  list <PlatAPI::User>::iterator i;
		  
		  PlatAPI::User *TempUserArray = new PlatAPI::User[];
		  PlatAPI::User *userArray = new PlatAPI::User[];     // Stores the number of users per getAuthCerts call

		  PlatAPI *createPlatAPI( Version version, Result &rc );

		  // DLL Interop bridge functions
		  extern "C" __declspec(dllexport) PlatAPI* CreatePlatformAPI()
						{
								return new PlatAPI();
						}

		  extern "C" __declspec(dllexport) void DisposePlatformAPI(PlatAPI* platformAPIPtr)
						{
								if ( platformAPIPtr != NULL )
									{
										  delete platformAPIPtr;
										  platformAPIPtr = NULL;
									}
						}

		  extern "C" __declspec(dllexport) int CallAuthenticateUser(PlatAPI* platformAPIPtr, 
		                                                            wchar_t* user, 
		                                                            wchar_t* password, 
		                                                            wchar_t* gamesku )
						{
								wstring userstr = user;
								wstring passwordstr = password;
								wstring gameskustr = gamesku;
								return platformAPIPtr->authenticateUser(userstr, passwordstr, gameskustr);
						}

		  extern "C" __declspec(dllexport) int CallGetAuthCertificates(PlatAPI* platformAPIPtr, 
		                                                                wchar_t** users,
		                                                                __int32* returnCodes,
		                                                                __int32* certCount)
						{
								//PlatAPI::User** userArray = 0;
								PlatAPI::User* userArray = 0;

								int result = platformAPIPtr->getAuthCerts(userArray, certCount);
								//int result = platformAPIPtr->getAuthCerts();

								users = new wchar_t*[*certCount];
								returnCodes = new __int32[*certCount];

								for( int i = 0; i < *certCount; i++ )
								{
								    //memcpy( users[i], userArray[i]->name().c_str(), userArray[i]->name().size() + 1 );
								    memcpy( users[i], userArray[i].name().c_str(), userArray[i].name().size() + 1 );
								    //returnCodes[i] = userArray[i]->User::returnCode();
								    returnCodes[i] = userArray[i].User::returnCode();
								    //delete userArray[i];
								}

								delete userArray;
								return result;
						}  
			}

