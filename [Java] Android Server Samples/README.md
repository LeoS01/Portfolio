## Intro
Context: Custom android-server

These are 2 parts of my small android server project: The networking layer and a small app hosted on the server.
The networking layer provides an abstract class that represents a HTTP/1.1 Server as a template-method pattern. Clients of the class can then override OS-Specific behaviour.
The tiny web-app just serves selected media from the phone.

## Known shortcomings
- The server code is not optimized for many concurrent users. However, the point of the android-app is to transform the smartphone into a small device that can act as a bridge between the users data on the phone and a browser. This situation already implies a very low concurrent user-count.
- Missing Status-Codes are yet to be added to the server. As i am the only user of the application as of today, this issue had a quite low priority.

## Reference
- Android Docs
- Java Docs
- AI was used as an aid for research. It helped summarize Docs, point to the right Packages and generate appropriate code-samples.