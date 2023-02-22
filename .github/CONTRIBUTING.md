## Contributing to nheko

Any kind of contribution to the project is greatly appreciated.

- Bug fixes
- Implementing new features
- UI/UX improvements/suggestions
- Code refactoring
- Translations

Usually we accept contributions on GitHub as Pull-Requests: https://github.com/Nheko-Reborn/nheko/pulls

Otherwise you can also submit patches, but that is more effort for both sides.

### Working on new features

Everything on the issue tracker is up for grabs unless someone else is 
currently working on it. 

If you're planning to work on a new feature leave a message on the Matrix room 
(or in the corresponding issue), so we won't end up having duplicate work.

### Submitting a translation

Usually translations are done on weblate: https://weblate.nheko.im/

Anyone can submit suggestions and once they have 2 upvotes, they will
automatically be applied. If you want to directly apply your or other changes
without going through the consensus mode, you can also jump into the Matrix room
and apply to become a translator.

Alternatively translations can also be added manually like so:

Example for a Japanese translation.
- Create a new translation file using the prototype in English
  - e.g `cp resources/langs/nheko_en.ts resources/langs/nheko_ja.ts`
- Open the new translation file and change the line regarding the locale to reflect the current language.
  - e.g `<TS version="2.1" language="en">` => `<TS version="2.1" language="ja">`
- Run `make update-translations` to update the translation files with any missing text.
- Fill out the translation file (Qt Linguist can make things easier).
- Submit a PR!


### Code style

We use clang-format to enforce a certain style as defined by the `.clang-format`
file in the root of the repo. Travis-CI will run the linter (macOS job) on each 
commit and the build will fail if the style guide isn't followed. You can run the
linter locally with `make lint`.


If you have any questions don't hesitate to reach out to us on #nheko:matrix.org.
