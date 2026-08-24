## Intro
Context: Custom android-server, intended as an introduction to network-based applications.

These are 2 parts of this project: The networking layer and a small app hosted on the server.
The networking layer provides an abstract class that represents a HTTP/1.1 Server within a template-method pattern. Clients of the class can thus override with OS-Specific behaviour.
The tiny web-app just serves selected media from the phone.

## Known shortcomings
- The server code is not optimized for many concurrent users. However, the point of the android-app is to transform the smartphone into a small device that can act as a bridge between the users data on the phone and a browser. This situation already implies a very low concurrent user-count.
- Missing Status-Codes are yet to be added to the server. As i am the only user of the application as of today, this issue had a quite low priority.
- As of today, the server only supports GET Requests. It does suffice for a simple usecase, such as streaming files from a phone. However, adding additional HTTP-Methods is planned and a work in progress (as seen in the code).

## Reference
- Android Docs
- Java Docs
- AI was used to aid in research. It helped by summarizing Docs, pointing to the right packages and by generating code-samples.