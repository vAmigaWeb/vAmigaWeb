// CodeMirror 6 bundle entry point
// Exposes all APIs needed by vAmigaWeb as global CM6

export {
  EditorState,
  StateEffect,
  Compartment
} from "@codemirror/state";

export {
  EditorView,
  lineNumbers,
  highlightActiveLine,
  keymap,
  placeholder,
  drawSelection,
  highlightSpecialChars,
  scrollPastEnd
} from "@codemirror/view";

export {
  javascript
} from "@codemirror/lang-javascript";

export {
  linter,
  lintGutter
} from "@codemirror/lint";

export {
  autocompletion,
  completionKeymap,
  closeBrackets,
  closeBracketsKeymap,
  startCompletion
} from "@codemirror/autocomplete";

export {
  bracketMatching,
  indentOnInput,
  syntaxHighlighting,
  defaultHighlightStyle,
  HighlightStyle
} from "@codemirror/language";

export { tags } from "@lezer/highlight";

export {
  defaultKeymap,
  history,
  historyKeymap,
  indentWithTab
} from "@codemirror/commands";
