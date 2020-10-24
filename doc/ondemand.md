
A Better Way to Parse Documents?
====================

Whether we parse JSON or XML, or any other serialized format, there are relatively few common strategies:

- The most established approach is the construction of document-object-model (DOM).
- Another established approach is a event-based approach (like SAX, SAJ). 
- Another popular approach is the schema-based deserialization model. 

We propose an approach that is as easy to use and often as flexible as the DOM approach, yet as fast and 
efficient as the schema-based or event-based approaches. We call this new approach "On Demand". The 
simdjson On Demand API offers a familiar, friendly DOM API and 
provides the performance of just-in-time parsing on top of the simdjson superior performance.

To achieve ease of use, we mimicked the *form* of a traditional DOM API: you can iterate over
arrays, look up fields in objects, and extract native values like `double`, `uint64_t`, `string` and `bool`.

To achieve performance, we introduced some key limitations that make the DOM API *streaming*:
array/object iteration cannot be restarted, and fields must be looked up in order, and string/number
values can only be parsed once. If these limitations are acceptable to you, the On Demand API could
help you write maintainable applications with a computation efficiency that is difficult to surpass.

A code example illustrates our API from a programmer's point of view:

```c++
ondemand::parser parser;
auto doc = parser.iterate(json);
for (auto tweet : doc["statuses"]) {
  std::string_view text        = tweet["text"];
  std::string_view screen_name = tweet["user"]["screen_name"];
  std::string_view screen_name;
  {
        ondemand::object user        = tweet["user"];
        screen_name                  = user["screen_name"];
  }
  uint64_t         retweets    = tweet["retweet_count"];
  uint64_t         favorites   = tweet["favorite_count"];
  cout << screen_name << " (" << retweets << " retweets / " << favorites << " favorites): " << text << endl;
}
```

Such code would be apply to a JSON document such as the following JSON mimicking a sample result from the Twitter API:

```json
{
	"statuses": [{
			"text": "@aym0566x \n\n名前:前田あゆみ\n第一印象:なんか怖っ！\n今の印象:とりあえずキモい。噛み合わない\n好きなところ:ぶすでキモいとこ😋✨✨\n思い出:んーーー、ありすぎ😊❤️\nLINE交換できる？:あぁ……ごめん✋\nトプ画をみて:照れますがな😘✨\n一言:お前は一生もんのダチ💖",
			"user": {
				"name": "AYUMI",
				"screen_name": "ayuu0123",
				"followers_count": 262,
				"friends_count": 252
			},
			"retweet_count": 0,
			"favorite_count": 0
		},
		{
			"text": "RT @KATANA77: えっそれは・・・（一同） http://t.co/PkCJAcSuYK",
			"user": {
				"name": "RT&ファボ魔のむっつんさっm",
				"screen_name": "yuttari1998",
				"followers_count": 95,
				"friends_count": 158
			},
			"retweet_count": 82,
			"favorite_count": 42
		}
	]
}
```

This streaming approach means that unused fields and values are not parsed or
converted, thus saving space and time. In our example, the `"name"`, `"followers_count"`,
and `"friends_count"` keys and matching values are skipped.

Further, the On Demand API does not parse a value *at all* until you try to convert it (e.g., to `double`,
`int`, `string`, or `bool`). In our example, when accessing the key-value pair `"retweet_count": 82`, the parser
may not convert the pair of characters `82` to the binary integer 82. Because the programmer specifies the data
type, we avoid branch mispredictions related to data type determination and improve the performance.


We expect users of an On Demand API to work in terms of a JSON dialect, which is a set of expectations and
specifications that come in addition to the [JSON specification](https://www.rfc-editor.org/rfc/rfc8259.txt). 
The On Demand approach is designed around several principles:

* **Streaming (\*):** It avoids preparsing values, keeping the memory usage and the latency down. 
* **Forward-Only:** To prevent reiteration of the same values and to keep the number of variables down (literally), only a single index is maintained and everything uses it (even if you have nested for loops). This means when you are going through an array of arrays, for example, that the inner array loop will advance the index to the next comma, and the array can just pick it up and look at it.
* **Natural Iteration:** A JSON array or object can be iterated with a normal C++ for loop. Nested arrays and objects are supported by nested for loops.
* **Use-Specific Parsing:** Parsing is always specific to the type required by the programmer. For example, if the programmer asks for an unsigned integer, we just start parsing digits. If there were no digits, we toss an error. There are even different parsers for `double`, `uint64_t` and `int64_t` values. This use-specific parsing avoids the branchiness of a generic "type switch," and makes the code more inlineable and compact.
* **Validate What You Use:** On Demand deliberately validates the values you use and the structure leading to it, but nothing else. The goal is a guarantee that the value you asked for is the correct one and is not malformed: there must be no confusion over whether you got the right value.



To understand why On Demand is different, it is helpful to review the major
approaches to parsing and parser APIs in use today.

### DOM Parsers

Many of the most usable, popular JSON APIs (including simdjson) deserialize into a **DOM**: an intermediate tree of
objects, arrays and values. In this model, we convert the input data all at once into a tree-like structure (the DOM). 
The DOM is then accessed by the programmer like any other in-memory data structure. The resulting API let 
you refer to each array or object separately, using familiar techniques like iteration (`for (auto value : array)`) 
or indexing (`object["key"]`). In some cases, the values are even deserialized directly into familiar C++ constructs like vectors and
maps.

The DOM approach is conceptually simple and "programmer friendly". Using the
DOM tree is often easy enough that many users use the DOM as-is instead of creating their own
their own custom data structures.

The DOM approach was the only way to parse JSON documents up to version 0.6 of the simdjson library.
Our DOM API looks similar to our On Demand example, except
it calls `parse` instead of `iterate`:

```c++
dom::parser parser;
auto doc = parser.parse(json);
for (auto tweet : doc["statuses"]) {
  std::string_view text        = tweet["text"];
  std::string_view screen_name = tweet["user"]["screen_name"];
  uint64_t         retweets    = tweet["retweet_count"];
  uint64_t         favorites   = tweet["favorite_count"];
  cout << screen_name << " (" << retweets << " retweets / " << favorites << " favorites): " << text << endl;
}
```

Pros of the DOM approach:
* Straightforward, programmer-friendly interface (arrays and objects).
* Safe: all of the input data has been validated before it is accessed.
* All of the JSON document is available at once to the programmer.

Cons of the DOM approach:
* The memory usage scales linearly with the size of the input document.
* Parses and stores everything, using memory and CPU cycles even on unused values.
* Performance drain from [type blindness](#type-blindness).


What the simdjson library demonstrates is that a DOM API may be quite fast indeed: we can parse files at speeds
of several gigabytes per second. However, in some instances, it may be possible to achieve even higher speeds.

### Event-Based Parsers (SAX, SAJ, etc.)


The event-based model (originally from the "Streaming API for XML") uses streaming to eliminate the cost of
parsing and storing the entire JSON. In the event-based model, a core JSON engine parses the JSON document
piece by piece, but instead of stuffing values in a DOM tree, it passes each value to a callback function,
letting the user decide for themselves how to handle it. In such a model, the programmer may need to provide functions 
for all possible events (a number, a string, a new object, a new array, the array ends, the object ends, and so on). 
This allows programmers to work with much larger files without running out of memory. 

The drawback is complexity: event-based APIs generally have you define a single callback for each type
(e.g. `string_field(std::string_view key, std::string_view value)`). Because of this, the programmer suffers
from context blindness: when they find a string they have to check where it is before they know what to
do with it. Is this string the text of the tweet, the screen name, or something else? Are we even in 
a tweet right now, or is this from some other place in the document
entirely? Though an event-based approach may allow superior performance, it is demanding of the programmer
who must efficiently keep track of its current state within the JSON input.

The following is event-based example of the Twitter problem we have reviewed in the DOM and On Demand
examples. To make it short enough to use as an example at all, it has heavily redacted: it only solves
a part of the problem (does not get user.screen_name), it has bugs (it does not handle sub-objects
in a tweet at all), and it uses a theoretical, simple event-based API that minimizes ceremony.

```c++
struct twitter_callbacks {
  bool in_statuses;
  bool in_tweet;
  std::string_view text;
  uint64_t         retweets;
  uint64_t         favorites;
  void start_object_field(std::string_view key) {
    if (key == "statuses") { in_statuses = true; }
  }
  void start_object() {
    if (in_statuses) { in_tweet = true; }
  }
  void string_field(std::string_view key, std::string_view value) {
    if (in_tweet && key == "text") { text = value; }
  }
  void number_field(std::string_view key, uint64_t value) {
    if (in_tweet) {
      if (key == "retweet_count") { retweets = value; }
      if (key == "favorite_count") { favorites = value; }
    }
  }
  void end_object() {
    if (in_tweet) {
      cout << "[redacted] (" << retweets << " retweets / " << favorites << " favorites): " << text << endl;
      in_tweet = false;
    } else if (in_statuses) {
      in_statuses = false;
    }
  }
};
sax::parser parser;
parser.parse(twitter_callbacks());
```

This is a large amount of code, requiring mental gymnastics even to read. An actual implementation is  harder to write 
and to maintain.


Pros of the event-based approach:
* Speed and space benefits from low, predictable memory usage.
* Parsing can be done more lazily: the API can delegate work to the programmer for better performance.
* It is highly flexible: given enough effort, most tasks can be accomplished efficiently.

Cons of the event-based approach:
* Performance drain from context blindness (e.g., switch statements for "where am I in the document")
* Difficult to use (high code complexity, high maintenance, difficult to debug)
* Lacks the safety of DOM: malformed documents could be ingested.

Though an event-based approach might have its niche uses, we believe that it is rarely ideally suited. We suspect that it is mostly used when performance and memory is a concern, and no other option (except DOM) is readily available.

### Schema-Based Parser Generators


In a schema-based model, the programmer provides a description of a data structure, and the parser constructs the data structure in question during parsing. These parsers take a schema--a description of
your JSON, with field names, types, everything--and generate classes/structs in your language of
choice, as well as a parser to deserialize the JSON into those structs. Some such parsers let you
define your own data structures (`struct`) and they let a preprocessor inspects it and generates a custom JSON parser for it.
Though not all of these schema-based parser generators generate a parser or even optimize for
streaming, but they are *able* to in principle. Unlike the DOM and the event-based models, a schema-based approach assumes
 that the structure of the document is known at compile-time. 


Pros of the schema-based approach:
* Ease of Use is on par with DOM
* Parsers that generate iterators and lazy values in structs can keep memory pressure down to event-based levels.
* Type Blindness can be entirely solved with specific parsers for each type, saving many branches.
* Context Blindness can be solved, especially if object fields are required and in order, saving even more branches.
* Can be made a safe as DOM: the input can be entirely validated prior to ingestion.

Cons of the schema-based approach:
* It is less flexible than the DOM or event-based approaches, sometimes limited to a deserialization-to-objects scenario.
* The structure of the data must be fully known at compile-time.


### Type Blindness and Branch Misprediction

The DOM and event-based parsing model suffer from **type
blindness**: even when the programmer knows exactly what fields and what types are in the JSON document,
the parser does not. This means it has to look at each value blind with a big "switch"
statement, asking "is this a number? A string? A boolean? An array? An object?"

In modern processors, this kind of switch statement can make your program run slower
than it needs to because of the high cost of branch misprediction. Indeed, modern processor 
cores rely on speculative execution for speed. They "read ahead" in your program, predicting 
which instructions to run as soon as the data is available. A single-threaded program can 
execute 2, 3 or even more instructions per cycle--largely because of speculative execution.

Unfortunately, when the processor mispredicts the instructions, typically due to a mispredicted
branch, all of the work done from the misprediction has be discarded and started anew. The
processor may have been executing 3 or 4 instructions per cycle, and consuming the corresponding
power, but all of the work may have been wasteful.

Type blindness means that the processor has to guess, for every JSON value, whether it will be an array,
an object, number, string or boolean since these correspond to distinct code paths.
Though some JSON files have predictable content, we find in practice that many JSON files
stress the branch prediction. Though branch predictors improve with each new generation of processors,
the cost of branch mispredictions also tends to increase as pipelines expand, and the processors become 
able to schedule longer streams of instructions.

On Demand parsing is tailor-made to solve this problem at the source, parsing values only after the
user declares their type by asking for a `double`, an `int`, a `string`, etc. It attempts to do so while
preserving most of the flexibility of DOM parsing.

Algorithm
---------

To help visualize the algorithm, we'll walk through the example C++ given at the top, for this JSON:

```json
{
  "statuses": [
    { "id": 1, "text": "first!", "user": { "screen_name": "lemire", "name": "Daniel" }, "favorite_count": 100, "retweet_count": 40 },
    { "id": 2, "text": "second!", "user": { "screen_name": "jkeiser2", "name": "John" }, "favorite_count": 2, "retweet_count": 3 }
  ]
}
```

### Starting the iteration

1. First, we declare a parser object that keeps internal buffers necessary for parsing. This can be
   reused to parse multiple JSON files, so you do not pay the high cost of allocating memory every
   time (and so it can stay in cache!).

   This declaration does not allocate any memory; that will happen in the next step.

   ```c++
   ondemand::parser parser;
   ```

2. We then start iterating the JSON document by allocating internal parser buffers, preprocessing
   the JSON, and initializing the iterator.

   ```c++
   auto doc = parser.iterate(json);
   ```
   
   Since this is the first time this parser has been used, `iterate()` first allocates internal
   parser buffers if this is the first time through. When reusing an existing parser, allocation
   only happens if the new document is bigger than internal buffers can handle. The On Demand 
   API only ever allocates memory in the `iterate()` function call.
   
   The simdjson library then preprocesses the JSON text at high speed, finding all tokens (i.e. the starting
   position of any JSON value, as well as any important operators like `,`, `:`, `]` or `}`).

   Finally, a `document` iterator is created, initialized at the position of the first value in the
   `json` text input. The document iterator is bumped forward by array / object iterators and
   object[] lookup, and must be kept around until iteration is complete.

   This operation can fail as this stage if the document in invalid! The result type is `simdjson_result<document>`.
   The simdjson library uses `simdjson_result` when a value needs to be returned by a function that can fail given improper inputs.
   The `simdjson_result` value contain an `error_code` and a `document`, and it was designed to allow you to use either error code
   checking or C++ exceptions via a direct cast `document(parser.iterate(json))` you can use `get()`
   to check the error and cast to a value, or cast directly to a value. However, the simdjson library
   rely on error chaining, so it is possible to delay error checks: we shall shortly explain error
   chaining more fully.

   NOTE: You should always have such a `document` instance (here `doc`) and it should remain in scope for the duration
   of your parsing function. E.g., you should not use the returned document as a temporary (e.g., `auto x = parser.iterate(json).get_object();`)
   followed by other operations as the destruction of the `document` instance makes all of the derived instances 
   ill-defined.


3. We iterate over the "statuses" field using a typical C++ iterator, reading past the initial
   `{ "statuses": [ {`.

   ```c++
   for (ondemand::object tweet : doc["statuses"]) {
   ```
  This shorthand does much, and it is helpful to see what it expands to.
  Comments in front of each one explain what's going on:
  ```c++
  // Validate that the top-level value is an object: check for {
  ondemand::object top = doc.get_object();

  // Find the field statuses by:
  // 1. Check whether the object is empty (check for }). (We do not really need to do this unless the key lookup fails!)
  // 2. Check if we're at the field by looking for the string "statuses" using byte-by-byte comparison.
  // 3. Validate that there is a `:` after it.
  auto tweets_field = top["statuses"];

  // Validate that the field value is an array: check for [
  // Also mark the array as finished if there is a ] next, which would cause the while () statement to exit immediately.
  ondemand::array tweets = tweets_field.get_array();
  // These three method calls do nothing substantial (the real checking happens in get_array() and ++)
  // != checks whether the array is marked as finished (if we have found a ]).
  ondemand::array_iterator tweets_iter = tweets.begin();
  while (tweets_iter != tweets.end()) {
    auto tweet_value = *tweets_iter;

    // Validate that the array element is an object: check for {
    ondemand::object tweet = tweet_value.get_object();
    ...
  }
  ```
   What is not explained in this code expansion is *error chaining*.
   Generally, you can use `document` methods on a `simdjson_result<...>` value; any errors will 
   just be passed down the chain. Many method calls
   can be chained in this manner. So `for (object tweet : doc["statuses"])`, which is the equivalent of
   `object tweet = *(doc.get_object()["statuses"].get_array().begin()).get_object()`, could fail in any of 
   6 method calls, and the error will only be checked at the end,
   when you attempt to cast the final `simdjson_result<object>` to object. Upon casting, an exception is
   thrown if there was an error.

   NOTE: while the document can be queried once for a key as if it were an object, it is not an actual object
   instance. If you need to treat it as an object (e.g., to query more than one keys), you can cast it as 
   such `ondemand::object root_object = doc.get_object();`.


4. We get the `"text"` field as a string.

   ```c++
   std::string_view text        = tweet["text"];
   ```

   First, `["text"]` skips the `"id"` field because it does not match: skips the key, `:` and
   value (`1`). We then check whether there are more fields by looking for either `,`
   or `}`.

   The second field is matched (`"text"`), so we validate the `:` and move to the actual value.

   NOTE: `["text"]` does a *raw match*, comparing the key directly against the raw JSON. This means
   that keys with escapes in them may not be matched and the letter case must match exactly.

   To convert to a string, we check for `"` and use simdjson's fast unescaping algorithm to copy
   `first!` (plus a terminating `\0`) into a buffer managed by the `document`. This buffer stores
   all strings from a single iteration. The next string will be written after the `\0`.

   A `string_view` is returned which points to that buffer, and contains the length.

4. We get the `"screen_name"` from the `"user"` object.

   ```c++
      ondemand::object user        = tweet["user"];
      screen_name                  = user["screen_name"];
   ```

   First, `["user"]` checks whether there are any more object fields by looking for either `,` or
  `}`. Then it matches `"user"` and validates the `:`.

   `["screen_name"]` then converts to object, checking for `{`, and finds `"screen_name"`.

   To convert the result to usable string (i.e., the screen name `lemire`), the characters are written to the document's 
   string buffer (after possibly escaping them), which now has *two* string_views pointing into it, and looks like `first!\0lemire\0`.

   Finally, the temporary user object is destroyed, causing it to skip the remainder of the object
   (`}`).

   NOTE: You may only have one active array or object active at any given time. An array or an object becomes
   active when the `ondemand::object` or `ondemand::array` is created, and it releases its 'focus' when
   its destructor is called. If you create an array or an object located inside a parent object or array,
   the child array or object becomes active while the parent becomes temporarily inactive. If you access
   several sibling objects or arrays, you must ensure that the destructor is called by scoping each access
   (see Iteration Safety section below for further details).

5. We get `"retweet_count"` and `"favorite_count"` as unsigned integers.

   ```c++
   uint64_t         retweets    = tweet["retweet_count"];
   uint64_t         favorites   = tweet["favorite_count"];
   ```

6. We loop to the next tweet.

   ```c++
   for (ondemand::object tweet : doc["statuses"]) {
     ...
   }
   ```

   The relevant parts of the loop  are:

   ```c++
   while (iter != statuses.end()) {
     ondemand::object tweet = *iter;
     ...
     iter++;
   }
   ```

   First, the `tweet` destructor runs, skipping the remainder of the object which in this case is
   just `}`.

   Next, `iter++` checks whether there are more values and finds `,`. The loop continues.

   Finally, `ondemand::object tweet = *iter` checks for `{` and returns the object.

   This tweet is processed just like the previous one.

7. We finish the last tweet.

   At the end of the loop, the `tweet` is first destroyed, skipping the remainder of the tweet
   object (`}`).
   
   The `iter++` instruction from `for (ondemand::object tweet : doc["statuses"])` then checks whether there are
   more values and finds that there are none (`]`). It marks the array iteration as finished and the for
   loop terminates.

   Then the outer object is destroyed, skipping everything up to the `}`.

Design Features
---------------

### String Parsing

When the user requests strings, we unescape them to a single string buffer much like the DOM parser
so that users enjoy the same string performance as the core simdjson. We do not write the length to the 
string buffer, however; that is stored in the `string_view` instance we return to the user.

```C++
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  std::set<std::string_view> default_users;
  ondemand::array tweets = doc["statuses"].get_array();
  for (auto tweet_value : tweets) {
    auto tweet = tweet_value.get_object();
    ondemand::object user = tweet["user"].get_object();
    std::string_view screen_name = user["screen_name"].get_string();
    bool default_profile = user["default_profile"].get_bool();
    if (default_profile) { default_users.insert(screen_name); }
  }
```

By using `string_view` instances, we avoid the high cost of allocating many small strings (as would be the
case with `std::string`) but be mindful that the life cycle of these `string_view` instances is tied to the
parser instance. If the parser instance is destroyed or reused for a new JSON document, these strings are no longer valid.

We iterate through object instances using `field` instances which represent key-value pairs. The value
is accessible by the `value()` method whereas the key is accessible by the `key()` method. 
The keys are treated differently than values are made available as as special type `raw_json_string` 
which is a lightweight type that is meant to be used on a temporary basis, amost solely for 
direct raw ASCII comparisons (`field.key() == "mykey"`). If you occasionally need to access and store the
unescaped key values, you may use the `unescaped_key()` method. Once you have called `unescaped_key()` method, 
neither the `key()` nor the `unescaped_key()` methods should be called: the current field instance
has no longer a key (that is by design). Like other strings, the resulting `std::string_view` generated
from the `unescaped_key()` method has a lifecycle tied to the `parser` instance: once the parser
is destroyed or reused with another document, the `std::string_view` instance becomes invalid.


```C++
auto doc = parser.iterate(json);
for(auto field : doc.get_object())  {
      std::string_view keyv = field.unescaped_key();
}
```

### Iteration Safety

The On Demand API is powerful. To compensate, we add some safeguards to ensure that it can be used without fear
in production systems:

  - If the value fails to be parsed as one type, the program can try to parse it as something else until the program succeeds. Thus 
    the programmer can engineer fall back routines.
  - If the value succeeds in being parsed or converted to a type, the program cannot try again. An attempt to parse the same node twice will
    cause the program to abort. We put this safety measure in the API to prevent double iteration of an array which
    would cause inconsistent iterator state or double-unescaping a string which may cause memory
    overruns if done.
  - Guaranteed Iteration: If you discard a value without using it--perhaps you just wanted to know
    if it was `nullptr` but did not care what the actual value was--it will iterate. The destructor automates
    the iteration.

  Some care is needed when using the On Demand API in scenarios where you need to access several sibling arrays or objects because
  only one object or array can be active at any one time. Let us consider the following example:

```C++
    ondemand::parser parser;
    const padded_string json = R"({ "parent": {"child1": {"name": "John"} , "child2": {"name": "Daniel"}} })"_padded;
    auto doc = parser.iterate(json);
    ondemand::object parent = doc["parent"];
    // parent owns the focus
    ondemand::object c1 = parent["child1"];
    // c1 owns the focus
    // 
    if(std::string_view(c1["name"]) != "John") { ... } 
    // c2 attempts to grab the focus from parent but fails
    ondemand::object c2 = parent["child2"];
    // c2 is now in an unsafe state and the following line would be unsafe
    // if(std::string_view(c2["name"]) != "Daniel") { return false; }
```

    A correct usage is given by the following example:

```C++
    ondemand::parser parser;
    const padded_string json = R"({ "parent": {"child1": {"name": "John"} , "child2": {"name": "Daniel"}} })"_padded;
    auto doc = parser.iterate(json);
    ondemand::object parent = doc["parent"];
    // At this point, parent owns the focus
    {
      ondemand::object c1 = parent["child1"];
      // c1 grabbed the focus from parent
      if(std::string_view(c1["name"]) != "John") { return false; } 
    }
    // c1 went out of scope, so its destructor was called and the focus
    // was handed back to parent.
    {
      ondemand::object c2 = parent["child2"];
      // c2 grabbed the focus from parent
      // the following is safe:
      if(std::string_view(c2["name"]) != "Daniel") { return false; }
    }
```

### Benefits of the On Demand Approach

We expect that the On Demand approach has many of the performance benefits of the schema-based approach, while providing a flexibility that is similar to that of the DOM-based approach.

* Faster than DOM in some cases. Reduced memory usage.
* Straightforward, programmer-friendly interface (arrays and objects).
* Highly expressive, beyond deserialization and pointer queries: many tasks can be accomplished with little code.

### Limitations of the On Demand Approach

The On Demand approach has  some limitations:

* Because it operates in streaming mode, you only have access to the current element in the JSON document. Furthermore, the document is traversed in order so the code is sensitive to the order of the JSON nodes in the same manner as an event-based approach (e.g., SAX).
* The On Demand approach is less safe than DOM: we only validate the components of the JSON document that are used and it is possible to begin ingesting an invalid document only to find out later that the document is invalid. Are you fine ingesting a large JSON document that starts with well formed JSON but ends with invalid JSON content?

There are currently additional technical limitations which we expect to resolve in future releases of the simdjson library:

* The simdjson library offers runtime dispatching which allows you to compile one binary and have it run at full speed on different processors, taking advantage of the specific features of the processor. The On Demand API does not have runtime dispatch support at this time. To benefit from the On Demand API, you must compile your code for a specific processor. E.g., if your processor supports AVX2 instructions, you should compile your binary executable with AVX2 instruction support (by using your compiler's commands). If you are sufficiently technically proficient, you can implement runtime dispatching within your application, by compiling your On Demand code for different processors.
* There is an initial phase which scans the entire document quickly, irrespective of the size of the document. We plan to break this phase into distinct steps for large files in a future release as we have done with other components of our API (e.g., `parse_many`).
* The On Demand API does not support JSON Pointer. This capability is currently limited to our core API.
* You should be mindful that the though your software might write the keys in a consistent manner, the [JSON specification](https://www.rfc-editor.org/rfc/rfc8259.txt) states that "JSON parsing libraries have been observed to differ as to whether or not they make the ordering of object members visible". The On Demand API will help the programmer handle unexpected JSON dialects by throwing an exception when the unexpected occurs, but the programmer is responsible for handling such cases: e.g., by rejecting the JSON input that does not follow the expected JSON dialect. We intend to help users who wish to use the On Demand API but require support for order-insensitive semantics, but in our current implementation support for out-of-order keys (if needed) must be provided by the programmer. Currently, one might proceed in the following manner as a fallback measure if keys can appear in any order:
```C++
    for (ondemand::object my_object : doc["mykey"]) {
      for (auto field : my_object) {
        if      (field.key() == "key_value1") { process1(field.value()); }
        else if (field.key() == "key_value2") { process2(field.value()); }
        else if (field.key() == "key_value3") { process3(field.value()); }
      }
    }
```

### Applicability of the On Demand Approach

At this time we recommend the On Demand API in the following cases:

1. The 64-bit hardware (CPU) used to run the software is known at compile time. If you need runtime dispatching because you cannot be certain of the hardware used to run your software, you will be better served with the core simdjson API. (This only applies to x64 (AMD/Intel). On 64-bit ARM hardware, runtime dispatching is unnecessary.)
2. The used parts of JSON files do not need to be validated and the layout of the nodes follows a strict JSON dialect. If you are receiving JSON from other systems, you might be better served with core simdjson API as it fully validates the JSON inputs and allows you to navigate through the document at will.
3. Speed and efficiency are of the utmost importance. Keep in mind that the core simdjson API is highly efficient so adopting the On Demand API is not necessary for high efficiency.
4. As a developer, you value a clean, flexible and maintainable API.

Good applications for the On Demand API might be: 

* You are working from pre-existing large JSON files that have been vetted. You expect them to be well formed according to a known JSON dialect and to have a consistent layout. For example, you might be doing biomedical research or machine learning on top of static data dumps in JSON.
* You have a closed system on predetermined hardware. Both the generation and the consumption of JSON data is within your system. Your team controls both the software that produces the JSON and the software the parses it, your team knows and control the hardware. Thus you can fully test your system.
* You are working with stable JSON APIs which have a consistent layout and JSON dialect.

## Checking Your CPU Selection

Given that the On Demand API does not offer runtime dispatching, your code is compiled against a specific CPU target. You should
verify that the code is compiled against the target you expect: `haswell` (AVX2 x64 processors), `westmere` (SSE4 x64 processors), `arm64` (64-bit ARM), `fallback` (others). Under x64 processors, many programmers will want to target `haswell` whereas under ARM,
most programmers will want to target `arm64`. The `fallback` is probably only good for testing purposes, not for deployment.

```C++
  std::cout << simdjson::builtin_implementation()->name() << std::endl;
```

If you are using CMake for your C++ project, then you can pass compilation flags to your compiler by using
the `CMAKE_CXX_FLAGS` variable:

```
cmake  -DCMAKE_CXX_FLAGS="-march=haswell" -B build_haswell
cmake --build build_haswell
```