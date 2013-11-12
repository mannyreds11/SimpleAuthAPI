/*
 *  Authentication Platform API
 *  Author: Manuel Rojas
 */

#include "stdafx.h"
#include <windows.h>
#include "PlatformAPI.h"

using namespace std;
using namespace PLAT;

const Version PLAT_VERSION     = 1;

ofstream logstream;

/* write_data - 
   Writes XML Response to LOGFILE.txt
*/
size_t  write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
			{
		  	char outfilename[FILENAME_MAX] = "LOGFILE.txt";
		 		static FILE *outfile;
		  	size_t written;
		  	outfile = fopen(outfilename,"w");
		  
		  	if (outfile == NULL) 
		  		{
		      	logstream << "couldnt open file" << endl;
		     		return -1;
		  		}

		  	written = fwrite(ptr,size,nmemb,outfile);
		  	fclose(outfile);
		  	return written;
			}

/* getIndent -
   Used in ParseXML function.
   Indentation for XML Parsing printing.
*/
const char * getIndent( unsigned int numIndents )
			{
				static const char * pINDENT = "                                      + ";
				static const unsigned int LENGTH = strlen( pINDENT );

				if ( numIndents > LENGTH ) numIndents = LENGTH;

				return &pINDENT[ LENGTH-numIndents ];
			}

/* ParseXML - 
   Uses tinyxml library.
   Parses the XML file and separates it into 6 different tags:
   1. Document
   2. Element
   3. Comment
   4. Unknown 
   5. Text (this is what we want)
   6. Declaration 

   "Text" content has the response given by Server.
   We will use this and perform string comparison to match the
   different Result Codes possibilities
*/
int ParseXML( TiXmlNode * pParent, unsigned int indent = 0 )
		{
			ostringstream buffer;
			string XMLValue;
			int ResultCode = 0;

			if ( !pParent ) 
				{
					logstream << "Shouldn't be here" << endl;
					return -1;
				}
			
			TiXmlText *pText;

			int t = pParent->Type();

			printf( "%s", getIndent( indent));

			switch ( t )
						{
							case TiXmlNode::TINYXML_DOCUMENT:
									printf( "Document" );
									break;

							case TiXmlNode::TINYXML_ELEMENT:
									printf( "Element \"%s\"", pParent->Value() );
									break;

							case TiXmlNode::TINYXML_COMMENT:
									printf( "Comment: \"%s\"", pParent->Value());
									break;

							case TiXmlNode::TINYXML_UNKNOWN:
									printf( "Unknown" );
									break;

							case TiXmlNode::TINYXML_TEXT:
									pText = pParent->ToText();
									//logstream << "pText = " << pText->Value() << endl;
									buffer << pText->Value();
									XMLValue = buffer.str();
									buffer.str("");

								if (!XMLValue.empty())
								{
									if (XMLValue.substr(10,1) == "h")
									{
									  //logstream << "Case User hasn't gotten ID" << endl;
									  logstream << "Case User hasn't gotten ID" << endl;
									  return RC_AUTH_ERROR;
									}

									if (XMLValue.substr(10,1) == "a")
									{
									  //logstream << "Case User and pass incorrect" << endl;
									  logstream << "Case User and pass incorrect" << endl;
									  return RC_AUTH_BAD_UNAME_PASSWD;
									}

									if (XMLValue.substr(10,1) == "B")
									{
									  //logstream << "Case User Banned" << endl;
									  logstream << "Case User Banned" << endl;
									  return RC_AUTH_ERROR;
									}

									if (XMLValue.substr(10,1) == "n")
									{
									  if (XMLValue.substr(16,1) == "S")
									  {
									    //logstream << "Case User not in Sku" << endl;
									    logstream << "Case User not in Sku" << endl;
									    return RC_AUTH_USER_NOTIN_GMSKU;
									  }
									  else 
									  {
									      //logstream << "Case User not in group" << endl;
									      logstream << "Case User not in group" << endl;
									      return RC_AUTH_USER_NOTIN_GROUP;
									  }
									}

									if (XMLValue.substr(0,1) == "B")
									{
									  //logstream << "Case Banned user" << endl;
									  logstream << "Case Banned user" << endl;
									  return RC_AUTH_BANNED_USER;
									}
									else
									{
									    //logstream << "Case RC_AUTH_OK" << endl;
									    logstream << "Case RC_AUTH_OK" << endl;
									    return RC_AUTH_OK;
									}
								}
							break;

							case TiXmlNode::TINYXML_DECLARATION:
									printf( "Declaration" );
									break;
					
							default:
									break;

    }
    printf( "\n" );

    TiXmlNode * pChild;

    for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
				{
				  int TempResult;
				  TempResult = ParseXML( pChild, indent+2 );
				  
				  if (TempResult == RC_AUTH_OK)
						{
						  return TempResult;
						}

				  if (TempResult == RC_AUTH_ERROR)
						{
						  return TempResult;	
						}

				  if (TempResult == RC_AUTH_BAD_UNAME_PASSWD)
						{
						  return TempResult;
						}

				  if (TempResult == RC_AUTH_BANNED_USER)
						{
						  return TempResult;
						}

				  if (TempResult == RC_AUTH_USER_NOTIN_GROUP)
						{
						  return TempResult;
						}

				  if (TempResult == RC_AUTH_USER_NOTIN_GMSKU)
						{
						  return TempResult;
						}
				  else 
				  		{
				      	logstream << "Wrong Result Code. Shouldn't be here" << endl;
				     		return RC_AUTH_NO_RESPONSE;
				      	//logstream << "Wrong Result Code. Shouldn't be here" << endl;
				  		}
				} 
}

/* ProcessXMLResponse - 
   Reads LOGFILE.txt (Contains XML Response from Server).
   Calls ParseXML (Parses Server's Response and returns the appropiate Result Code).
   Returns Result Code to SendUserRequest function.
*/

int ProcessXMLResponse (const char *filename, const wstring & username, const wstring & password, const wstring & GameSKU)
{
    int RC = 0;
    unsigned int indent = 0;

    TiXmlDocument doc( filename );
    bool loadOkay = doc.LoadFile();

    if ( loadOkay )
		  {
		    RC = ParseXML( &doc );
		    //logstream << "Result Code = " << RC << endl;
		  }
    else
				{
				  printf( "Something went wrong\n" );
				}

    return RC;
}

/* SendUserRequest -
   Uses CURL Library to talk to the server.
   POST form is created and username, password and gamesku are posted to the server.
   Response from server is stored in LOGFILE.txt file thought write_data function.
*/

int SendUserRequest (const wstring & username, const wstring & password, const wstring & GameSKU) 
		{
		  CURL * curl;
		  CURLcode res;

		  ifstream read; 
		  wstringstream BufferURL;
		  wstring TempURL;
		  string XMLResponse;
		  int ResultCode;

		  struct curl_httppost *formpost = NULL;
		  struct curl_httppost *lastptr = NULL;
		  struct curl_slist *headerlist = NULL;
		  static const char buf[] = "Expect:";

		  // Initiate CURL easy handle. Always use curl_easy_cleanup afterwards when operation is completed. See below 
		  curl = curl_easy_init ();

		  headerlist = curl_slist_append (headerlist, buf);

		  BufferURL << "http://type_your_address_here";
		  BufferURL << username;
		  BufferURL << "&password=";
		  BufferURL << password;
		  BufferURL << "&gamesku=";
		  BufferURL << GameSKU;

		  //URL = BufferURL._Stdstr;
		  BufferURL >> TempURL;
		  string URL(TempURL.begin(), TempURL.end());

		  //logstream << "URL: " << URL << endl;

		  if (curl)
				{
				  // URL that receives this POST 
				  //curl_easy_setopt (curl, CURLOPT_URL,"http://type_your_address_here");
				  curl_easy_setopt (curl, CURLOPT_URL,URL.c_str());
				  curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headerlist);
				  curl_easy_setopt (curl, CURLOPT_HTTPPOST, formpost);

				  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, &write_data); 

				  res = curl_easy_perform (curl);

				  // Close all connections the handle has used
				  curl_easy_cleanup (curl);
				  curl_formfree (formpost);
				  curl_slist_free_all (headerlist);
				}

		  // Check if there was a succesful response from the server
		  if(CURLE_OK != res) 
				{
				  // Response failed. Return RC_AUTH_NO_RESPONSE
				  logstream << "Could not get a response from user" << endl;
				  ResultCode = RC_AUTH_NO_RESPONSE;
				  logstream << "RC = " << ResultCode << endl;
				  return ResultCode;
				}
		  else 
					{
						ResultCode = ProcessXMLResponse ("LOGFILE.txt", username, password, GameSKU);
						logstream << "RC = " << ResultCode << endl;
						return ResultCode;
					}


}

class TGI_PLAT::PlatAPI::User_Impl 
		{
			public: 
      User_Impl ():_account (0){}

      User_Impl (wstring name, wstring password, wstring gamesku):_account (0)
      {       
          _name = name; 
          _password = password;
          _gamesku = gamesku;
          wcout << "_name = " << _name << endl;
          wcout << "_password = " << _password << endl;
          wcout << "_gamesku = " << _gamesku << endl << endl;
      }

      PlatAPI::AccountID account ()const
      {
          return _account;
      }

      wstring setName (wstring name)
      {
          _name = name;
          wcout << "_name = " << _name << endl << endl;
          return _name;
      }

      wstring name () const
      {
          return _name;
      }

      wstring setPassword (wstring password)
      {
          _password = password;
          wcout << "_password = " << _password << endl << endl;
          return _password;
      }

      wstring password () const
      {
          return _password;
      }

      wstring setGamesku (wstring gamesku)
      {
          _gamesku = gamesku;
          wcout << "_gamesku = " << _gamesku << endl << endl;
          return _gamesku;
      }

      wstring gamesku () const
      {
          return _gamesku;
      }

      const vector < wstring > permissions () const
      {
          return _perms;
      }

      Result setReturnCode (Result response)
      {
          _returncode = response;
          wcout << "_returncode = " << _returncode << endl << endl;
          return _returncode;
      }

      Result returnCode () const
      {
          return _returncode;
      }

			private: 
      typedef vector < wstring > Permissions;
      PlatAPI::AccountID _account;
      wstring _name;
      wstring _password;
      wstring _gamesku;
      Permissions _perms;
      Result _returncode;
};


class TGI_PLAT::PlatAPI_Impl 
		{
			public:
			PlatAPI_Impl ()
			{
				// initialize curl
				// curl_global_init (CURL_GLOBAL_ALL);
				// boost::thread PumpThread(boost::bind(&this->PumpFunction));
			};

			/*
			PlatAPI::User * Auth (Result & rc)
			{
				rc = RC_AUTH_ERROR;
				return 0;
			};
			*/

			Result authenticateUser (const wstring & user, const wstring & password, const wstring & gamesku)
						{			
							PlatAPI::User MyUser;
							MyUser.setName(user);
							MyUser.setPassword(password);
							MyUser.setGamesku(gamesku);

							boost::mutex::scoped_lock lock(ResponseQueueResource);

							RequestQueue.push_front(MyUser);
							return 0;
							//return SendUserRequest (user, password, gamesku);
						}

			/* 
				 getAuthCerts - 
				 Receives an array of "users" from the Response Queue along with a certCount 
				 variable (total users received when called). The function will write the 
				 "users" list to the userArray[]. This userArray will be used by C# function 
				 "CallGetAuthCertificates" to retrieve the "users" information
			*/

			Result getAuthCerts(PlatAPI::User* users, int* certCount)
						{ 
							int TotalUsersCertified = 0;
							int j = 0;
							int start = 0;
							int end = 0;
							int temp = 0;
							logstream << "PreviousCertCount = " << PreviousCertCount << endl;
			
							certCount = new int();
			
							{
								logstream << "Locking the Response Queue" << endl;
								// lock outgoing queue critical section
								boost::mutex::scoped_lock lock(ResponseQueueResource);
								boost::mutex::scoped_lock lockio(io_mutex);

								logstream << "Response Queue locked" << endl;

								*certCount = ResponseQueue.size();
								logstream << "Response Queue size : " << *certCount << endl;

								users = new PlatAPI::User[*certCount];
								if( users )
								{
									logstream << "users array created" << endl;
								}
								else
								{
									logstream << "Failed to create user array" << endl;
								}
			
								j = 0;
								for ( i = ResponseQueue.begin(); i != ResponseQueue.end(); ++i)
								{
									memcpy(&users[j], &(*i), sizeof(*i));
					
									if (users[j].returnCode() == RC_AUTH_OK)
									{
										wcout << users[j].name() << " Certified" << endl  << endl;
										TotalUsersCertified++;
									}
									j++;
								}
			
								PreviousCertCount = usercounter;

								logstream << "Clearing the Response Queue" << endl;
								ResponseQueue.clear();
							}

							logstream << "Users Certified = " << TotalUsersCertified << endl;
							return TotalUsersCertified;
						}
		};

PlatAPI::User::User () { _impl = new User_Impl (); }

PlatAPI::User::User (wstring myname, wstring mypassword, wstring mygamesku ) { _impl = new User_Impl (myname,mypassword, mygamesku); }

PlatAPI::AccountID PlatAPI::User::account ()const { return _impl->account (); }

wstring PlatAPI::User::setName (wstring name) { return _impl->setName (name); }

wstring PlatAPI::User::name () const { return _impl->name (); }

wstring PlatAPI::User::setGamesku (wstring gamesku) { return _impl->setGamesku (gamesku); }

wstring PlatAPI::User::gamesku () const { return _impl->gamesku (); }

wstring PlatAPI::User::setPassword (wstring password) { return _impl->setPassword (password); }

wstring PlatAPI::User::password () const { return _impl->password (); }

const vector <wstring > PlatAPI::User::permissions () const { return _impl->permissions (); }

Result PlatAPI::User::setReturnCode(Result response) { return _impl->setReturnCode (response); }

Result PlatAPI::User::returnCode() const { return _impl->returnCode(); }


PlatAPI::PlatAPI ()
				{
				 // open the logstream stream for debug purposes
					logstream.open("platformAPILog.txt");

					logstream << "Created Log" << endl;

					_impl = new PlatAPI_Impl ();
					logstream << "Created PlatAPI_Impl object" << endl;

					curl_global_init (CURL_GLOBAL_ALL);
					logstream << "Initialized Curl" << endl;

					boost::thread PumpThread(boost::bind(&PlatAPI::PumpFunction, this));
					logstream << "Initialized API Pump Thread" << endl;	 
				};

PlatAPI::~PlatAPI()
				{
					logstream << "Cleaning up the PlatAPI object" << endl;
					if (_impl != NULL )
						{
							delete _impl;
						}

					logstream << "Closing the Log" << endl;
					logstream.close();
				};    


Result PlatAPI::authenticateUser (const wstring & user, const wstring & password, const wstring & GameSKU)
			{
				Result status = 0;
				logstream << "Authenticate User called" << endl;
				status = _impl->authenticateUser (user, password, GameSKU);
				logstream << "Returned from impl->authenticateUser, status = " << status << endl;
				return status;
			};


Result PlatAPI::getAuthCerts (User* users, int* certCount)
			{
				Result status = 0;
				logstream << "getAuthCerts called" << endl;

				if( certCount )
					{
						logstream << "certCount != null, certCount == " << *certCount << endl;
					}
				else
						{
							logstream << "certCount == null" << endl;
						}
				status = _impl->getAuthCerts (users, certCount);
				logstream << "Returned from impl->getAuthCerts, status = " << status << endl;

				if( certCount )
					{
						logstream << "certCount != null, certCount == " << *certCount << endl;
					}
				else
						{
							logstream << "certCount == null" << endl;
						}

				if( users )
					{
						logstream << "users != null\n" << endl;
					}
				else
						{
							logstream << "users == null\n" << endl;
						}
				return status;
			};


PlatAPI * createPlatAPI (Version version, Result & rc)
			{
				logstream << "Creating PlatAPI, version: " << version << endl;
				if (version > TGI_PLAT_VERSION)
					{
							rc = RC_AUTH_BAD_VERSION;
							return 0;
					}
				return new PlatAPI();
			}

/* 
   PumpFunction - 
   Function used in thread will receive User Requests and perform the 
   Authentication through authenticateUser() function. Then it will
   store the User information along with the Server Response (RC)
   into the Response Queue.
   PumpFunction will loop indefinetely waiting for User Requests.
   It will sleep as long as there are no Requests on the Queue.
*/

void PlatAPI::PumpFunction()
		{
			logstream << "Pump: Start Pumping" << endl;
			User *MyTempUser = new PlatAPI::User(L"",L"",L"");

			Result RC;

			//int usercounter = 0;

			while (1)
						{
							if (!RequestQueue.empty())
								{     
									if (!RequestQueue.empty())
										{
											// Pull elements out of the RequestQueue
											logstream << "Pump: locking request queue" << endl;
											boost::mutex::scoped_lock lock(RequestQueueResource); 
											logstream << "Pump: request queue locked" << endl;
											*MyTempUser = RequestQueue.front();
							
											//Remove User from the Request Queue 
											RequestQueue.pop_front();
							
											// output for debug to show user data
											boost::mutex::scoped_lock lockio(io_mutex);
											wcout << "*MyTempUser->name = " << MyTempUser->name() << endl;
											wcout << "*MyTempUser->password = " << MyTempUser->password() << endl;
											wcout << "*MyTempUser->gamesku = " << MyTempUser->gamesku() << endl << endl;
															  
										}
							
									// Authenticate User 
									//RC = this->authenticateUser(MyTempUser->name(), MyTempUser->password(), MyTempUser->gamesku());
									logstream << "Pump: sending the user request" << endl;
									RC = SendUserRequest (MyTempUser->name(), MyTempUser->password(), MyTempUser->gamesku());
					
									// Push elements out to the ResponseQueue
									{
										logstream << "Pump: Locking the response queue" << endl;
										boost::mutex::scoped_lock lock2(ResponseQueueResource);
										logstream << "Pump: Response queue locked" << endl;
													        
										// Save Response Value (Result Code) to User object
										MyTempUser->setReturnCode(RC);

										logstream << "Pump: adding request to the response queue" << endl;
										// Insert User obect into ResponseQueue
										ResponseQueue.push_back(*MyTempUser);
									}
								 
									logstream << "Pump: usercounter = " << usercounter << endl;
									usercounter++;                   
					
								}
							logstream << "Pump: sleep..." << endl;
							Sleep (1000);
						}
			/*
			boost::xtime t;
			boost::xtime_get(&t, 2);
			boost::thread::sleep(t);
			*/
			/*
			boost::xtime xt; 
			boost::xtime_get(&xt, boost::TIME_UTC); 
			xt.sec += 1; 
			boost::thread::sleep(xt);
			*/

		}


// For testing purposes...
void GenerateRequests()
		{
		 PlatAPI::User MyUser[100];

		 int x = 0;
		 for (x = 0; x < 5; x++)
			  {
	       MyUser[x].setName(L"username");
	       MyUser[x].setPassword(L"password");
	       MyUser[x].setGamesku(L"Gamesku");
	       RequestQueue.push_front(MyUser[x]);
			  }
		}

void GenerateRequests2()
		{
		 PlatAPI::User MyUser[100];

		 int x = 0;
		 for (x = 0; x < 3; x++)
			  {
	       MyUser[x].setName(L"username");
	       MyUser[x].setPassword(L"password");
	       MyUser[x].setGamesku(L"Gamesku");
	       RequestQueue.push_back(MyUser[x]);
			  }
		}

void GenerateRequests3()
		{
			PlatAPI::User MyUser[100];

			int x = 0;
			for (x = 0; x < 5; x++)
					{
					 MyUser[x].setName(L"username");
					 MyUser[x].setPassword(L"password");
					 MyUser[x].setGamesku(L"Gamesku");
					 RequestQueue.push_back(MyUser[x]);
					}
		}
