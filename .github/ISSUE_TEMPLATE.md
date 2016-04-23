## Precheck: My Sass isn't compiling
- [ ] Check if you can reproduce the issue via [SourceMap Inspector] [5] (updated regularly).
- [ ] Validate official ruby sass compiler via [SassMeister] [6] produces your expected result.
- [ ] Search for similar issue in [LibSass] [1] and [node-sass] [2] (include closed tickets)
- [ ] Optionally test your code directly with [sass] [7] or [sassc] [2] ([installer] [4])

## Precheck: My build/install fails
- [ ] Problems with building or installing libsass should be directed to implementors first!
- [ ] Except for issues directly verified via sassc or LibSass own build (make/autotools9

## Craft a meaningfull error report
- [ ] Include the version of libsass and the implementor (i.e. node-sass or sassc)
- [ ] Include information about your operating system and environment (i.e. io.js)
- [ ] Either create a self contained sample that shows your issue ...
- [ ] ... or provide it as a fetchable (github preferred) archive/repo
- [ ] ... and include a step by step list of command to get all dependencies
- [ ] Make it clear if you use indented or/and scss syntax

## My error is hiding in a big code base
1. we do not have time to support your code base!
2. to fix occuring issues we need precise bug reports
3. the more precise you are, the faster we can help you
4. lazy reports get overlooked even when exposing serious bugs
5. it's not hard to do, it only takes time
- [ ] Make sure you saved the current state (i.e. commit to git)
- [ ] Start by uncommenting blocks in the initial source file
- [ ] Check if the problem is still there after each edit
- [ ] Repeat until the problem goes away
- [ ] Inline imported files as you go along
- [ ] Finished once you cannot remove more
- [ ] The emphasis is on the word "repeat" ...

## What makes a code test case

Important is that someone else can get the test case up and running to reproduce it locally. For this
we urge you to verify that your sample yields the expected result by testing it via [SassMeister] [6]
or directly via ruby sass or node-sass (or any other libsass implementor) before submitting your bug
report. Once you verified all of the above, you may use the template below to file your bug report.

### Title: Be as meaningfull as possible in 60 chars if possible

input.scss
```scss
test {
  content: bar
}
```

libsass 3.5.5
```css
test {
  content: bar; }
```

ruby sass 3.4.21
```css
test {
  content: bar; }
```

version info:
```cmd
$ node-sass --version
node-sass       3.3.3   (Wrapper)       [JavaScript]
libsass         3.2.5   (Sass Compiler) [C/C++]
```

[1]: https://github.com/sass/libsass/issues?utf8=%E2%9C%93&q=is%3Aissue
[2]: https://github.com/sass/node-sass/issues?utf8=%E2%9C%93&q=is%3Aissue
[3]: https://github.com/sass/sassc
[4]: http://libsass.ocbnet.ch/installer/
[5]: http://libsass.ocbnet.ch/srcmap/
[6]: http://www.sassmeister.com/
[7]: https://rubygems.org/gems/sass