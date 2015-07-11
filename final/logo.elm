import Char
import Color
import Graphics.Collage
import Graphics.Element
import Graphics.Input.Field
import Keyboard
import Set
import Signal
import String
import Time

type alias Position =
    { x : Float, y : Float }

type alias Character =
    { currentPosition : Position , currentAngle : Float }

type alias Program =
    { turtle : Character, code : String, currentState : State, debug : String, lastCode : String }

-- LOGIC

width : Float
width = 500

intwidth : Int
intwidth = truncate width

height : Float
height = 500

intheight : Int
intheight = truncate height

-- TURTLE

defaultStep : Float
defaultStep = 10

defaultRotation : Float
defaultRotation = pi/16

currentProgram : Program
currentProgram = { turtle = { currentPosition = { x = -10, y = -10 }, currentAngle = pi/2 }, code = "", currentState = initial_state, debug = "Program not ok", lastCode = "" }

update : (Time.Time, Set.Set Keyboard.KeyCode, Bool) -> Program -> Program
update (delta, keycodes, backspace) prog =
    updateCode prog keycodes
    |> cutCode backspace
    |> validateCode
    |> updateExecution

updateCode : Program -> Set.Set Keyboard.KeyCode -> Program
updateCode prog keycodes =
    { prog | code <- concatenate prog.code (Set.toList keycodes)}

cutCode : Bool -> Program -> Program
cutCode pressed prog =
    if pressed then {prog | code <- (String.slice 0 -1 prog.code)}
    else prog

concatenate : String -> List Keyboard.KeyCode -> String
concatenate prev keycodes =
    case keycodes of
        [] -> prev
        hd::tl ->
            let x = unicodeToSring hd in
                if x=="BACKSPACE" then concatenate (String.slice 0 -1 prev) tl
                else concatenate (prev++x) tl

unicodeToSring : Keyboard.KeyCode -> String
unicodeToSring k =
    if | k == 65 -> "A"
       | k == 66 -> "B"
       | k == 67 -> "C"
       | k == 68 -> "D"
       | k == 69 -> "E"
       | k == 70 -> "F"
       | k == 71 -> "G"
       | k == 72 -> "H"
       | k == 73 -> "I"
       | k == 74 -> "J"
       | k == 75 -> "K"
       | k == 76 -> "L"
       | k == 77 -> "M"
       | k == 78 -> "N"
       | k == 79 -> "O"
       | k == 80 -> "P"
       | k == 81 -> "Q"
       | k == 82 -> "R"
       | k == 83 -> "S"
       | k == 84 -> "T"
       | k == 85 -> "U"
       | k == 86 -> "V"
       | k == 87 -> "W"
       | k == 88 -> "X"
       | k == 89 -> "Y"
       | k == 90 -> "Z"
       | k == 32 -> " "
       | k == 49 -> "1"
       | k == 50 -> "2"
       | k == 51 -> "3"
       | k == 52 -> "4"
       | k == 53 -> "5"
       | k == 54 -> "6"
       | k == 55 -> "7"
       | k == 56 -> "8"
       | k == 57 -> "9"
       | k == 48 -> "0"
       | k == 219 -> "["
       | k == 221 -> "]"
       | k == 17 -> "BACKSPACE"
       | otherwise -> ""


updateExecution : Program -> Program
updateExecution prog =
    case prog.currentState.line of
        [] -> prog
        hd::tl ->
            removeFirstPosition prog
            |> moveTurtle hd

removeFirstPosition : Program -> Program
removeFirstPosition prog = { prog | currentState <- updateLine prog.currentState}

updateLine : State -> State
updateLine state = {state | line <- dropFirst (state.line)}

dropFirst : List a -> List a
dropFirst lst =
    case lst of
        [] -> []
        hd::tl -> tl

moveTurtle : (Float, Float) -> Program -> Program
moveTurtle (a,b) prog =
    let prog2 = {prog | turtle <- checkRotation prog.turtle (a,b)} in
        { prog2 | turtle <- updateCharacter (prog2.turtle) (a,b)}

checkRotation : Character -> (Float, Float) -> Character
checkRotation char (a,b)= { char | currentAngle <- atan2 (b-char.currentPosition.y) (a-char.currentPosition.x)}

updateCharacter : Character -> (Float, Float) -> Character
updateCharacter char (a,b) = { char | currentPosition <- { x = a, y = b}}

rotateTurtleClockwise : Character -> Character
rotateTurtleClockwise turtle =
    { turtle | currentAngle <- (turtle.currentAngle - defaultRotation) }

rotateTurtleAntiClockwise : Character -> Character
rotateTurtleAntiClockwise turtle =
    { turtle | currentAngle <- (turtle.currentAngle + defaultRotation) }

moveFoward : Character -> Character
moveFoward character =
    { character | currentPosition <- updatePosition character.currentPosition character.currentAngle defaultStep}

updatePosition : Position -> Float -> Float -> Position
updatePosition pos tetha step =
    { x = pos.x+((cos tetha)*step), y = pos.y+((sin tetha)*step)}

-- GRAPHICS ~ TURTLE

render : Program -> Graphics.Element.Element
render prog =
  Graphics.Element.flow Graphics.Element.right (
    Graphics.Element.flow Graphics.Element.outward (
      (Graphics.Collage.collage intwidth intheight ((getTurtleForm prog.turtle)::[]))::[])
    ::
    Graphics.Element.flow Graphics.Element.down ((Graphics.Element.show prog.code)::(Graphics.Element.show prog.debug)::[])
    ::[])

getTurtleForm : Character -> Graphics.Collage.Form
getTurtleForm turtle =
    Graphics.Collage.toForm (Graphics.Element.image 200 200 "./seaturtle.jpeg")
    |> Graphics.Collage.rotate turtle.currentAngle
    |> Graphics.Collage.move (turtle.currentPosition.x, turtle.currentPosition.y)

-- SIGNALS

main : Signal Graphics.Element.Element
main =
    turtleSignal

turtleSignal : Signal Graphics.Element.Element
turtleSignal = (Signal.map render (Signal.foldp update currentProgram input))

input : Signal (Time.Time, Set.Set Keyboard.KeyCode, Bool)
input =
    Signal.sampleOn delta (Signal.map3 (,,) delta Keyboard.keysDown Keyboard.shift)

delta : Signal Time.Time
delta =
    Signal.map (\t -> t / 20) (Time.fps 5)

-- Utility

takeWhile : (Char -> Bool) -> String -> String
takeWhile f s =
  case String.uncons s of
  Nothing -> ""
  Just (x, xs) -> if f x then takeWhile f xs |> String.cons x
                         else ""

dropWhile : (Char -> Bool) -> String -> String
dropWhile f s =
  case String.uncons s of
  Nothing -> ""
  Just (x, xs) -> if f x then dropWhile f xs
                         else String.cons x xs

readInt : String -> Maybe Int
readInt s =
  let
    digits = takeWhile Char.isDigit s
  in
    String.toInt digits
    |> Result.toMaybe

-- Generic Parser

type Parser a = Parser (String -> Maybe (a, String))

-- Monad implementation for Parser
return : a -> Parser a
return a =
  Parser (\s -> Just (a, s))

(>>=) : Parser a -> (a -> Parser b) -> Parser b
pa >>= f =
  Parser (\s -> (parse pa s) `Maybe.andThen` (\(a, s2) -> parse (f a) s2 ))

(>>) : Parser a -> Parser b -> Parser b
pa >> pb = pa >>= (\_ -> pb)

-- Parser execution
parse : Parser a -> String -> Maybe (a, String)
parse (Parser f) s = f s

-- Basic parsers
rejectAll : Parser a
rejectAll =
  Parser (\s -> Nothing)

expectChar : Char -> Parser Char
expectChar c =
  let
    f = \(x, xs) -> if x == c then Just (x, xs) else Nothing
  in
    Parser (\s -> String.uncons s `Maybe.andThen` f)

expectString : String -> Parser String
expectString s =
  case String.uncons s of
  Nothing -> return ""
  Just (x, xs) -> (expectChar x)
              >>= (\c -> (expectString xs)
                     >>= (\s2 -> return (String.cons c s2)))

swallowWhitespace : Parser ()
swallowWhitespace =
  Parser (\s -> Just ((), String.trimLeft s))

(||) : Parser a -> Parser a -> Parser a
pa || pb =
  Parser (\s -> case parse pa s of
    Nothing -> parse pb s
    Just a -> Just a)

parseMany : Parser a -> Parser (List a)
parseMany pa =
  Parser (\s ->
    case parse pa s of
      Nothing ->
        Just ([], s)

      Just (a, os) ->
        parse (parseMany pa) os `Maybe.andThen` (\(lcmd, oos) -> Just (a :: lcmd, oos)))

parseInt : Parser Int
parseInt =
  swallowWhitespace
  >> Parser (\s -> readInt s |> Maybe.map (\n -> (n, dropWhile Char.isDigit s)))

-- Language specific definitions

type alias State =
  {
    pos : (Float, Float)
    , rot : Float
    , line : List (Float, Float)
  }

type alias Value = Int

type Exp
  = ExpK Value
  | ExpAdd Exp Exp
  | ExpSub Exp Exp

type Cmd
  = CmdFwd Exp
  | CmdRot Exp
  | CmdRepeat Exp Cmd
  | CmdBlock (List Cmd)

evalExp : Exp -> Value
evalExp exp =
  case exp of
    (ExpK v) -> v
    (ExpAdd e1 e2) -> (evalExp e1) + (evalExp e2)
    (ExpSub e1 e2) -> (evalExp e1) - (evalExp e2)

evalCmd : Cmd -> State -> State
evalCmd cmd state =
  case cmd of
    (CmdFwd exp) ->
      let
        dist = evalExp exp |> toFloat
        distX = cos (degrees state.rot) * dist
        distY = sin (degrees state.rot) * dist
        newPos = ( fst state.pos + distX, snd state.pos + distY )
      in
        { state | pos <- newPos, line <- List.append state.line [newPos] }

    (CmdRot exp) ->
      let
        angle = evalExp exp |> toFloat
      in
        { state | rot <- state.rot + angle }

    (CmdRepeat exp cmd) ->
      case evalExp exp of
        0 -> state
        n -> evalCmd (CmdRepeat (ExpK (n - 1)) cmd) (evalCmd cmd state)

    (CmdBlock l) ->
      evalCmdList l state

evalCmdList : List Cmd -> State -> State
evalCmdList l state =
  case l of
    [] -> state
    cmd :: cmds -> evalCmdList cmds (evalCmd cmd state)

-- Language specific parsers

parseVal : Parser Value
parseVal = parseValInt

parseValInt : Parser Value
parseValInt =
  swallowWhitespace
  >> parseInt
  >>= return

parseExp : Parser Exp
parseExp
  =  parseExpBin ExpAdd '+'
  || parseExpBin ExpSub '-'
  || parseExpK

parseExpK : Parser Exp
parseExpK =
  swallowWhitespace
  >> parseVal
  >>= (\val -> return (ExpK val))

parseExpBin : (Exp -> Exp -> Exp) -> Char -> Parser Exp
parseExpBin constructor c =
  swallowWhitespace
  >> parseVal
  >>= (\val -> swallowWhitespace
               >> expectChar c
               >> parseExp
               >>= (\exp -> return (constructor (ExpK val) exp)))

parseCmd : Parser Cmd
parseCmd
  =  parseCmdUn CmdRot "ROT "
  || parseCmdUn CmdFwd "FWD "
  || parseCmdRepeat
  || parseCmdBlock

parseCmdUn : (Exp -> Cmd) -> String -> Parser Cmd
parseCmdUn constructor token =
  swallowWhitespace
  >> expectString token
  >> swallowWhitespace
  >> parseExp
  >>= (\exp -> return (constructor exp))

parseCmdRepeat : Parser Cmd
parseCmdRepeat =
  swallowWhitespace
  >> expectString "REPEAT "
  >> swallowWhitespace
  >> parseExp
  >>= (\exp -> parseCmd >>= (\cmd -> return (CmdRepeat exp cmd)))


parseCmdBlock : Parser Cmd
parseCmdBlock =
  swallowWhitespace
  >> expectChar '['
  >> parseMany (parseCmdUn CmdRot "ROT " || parseCmdUn CmdFwd "FWD ")
  >>= (\lcmd ->
    swallowWhitespace
    >> expectChar ']'
    >> return (CmdBlock lcmd))

-- App logic

initial_state : State
initial_state = { pos = (0, 0), rot = 0, line = [(0, 0)] }

validateCode : Program -> Program
validateCode prog =
  if prog.lastCode == prog.code then prog
  else
    let newProg = { prog | lastCode <- prog.code } in
      let
        parserResult = parse (parseMany parseCmd) prog.code
      in
        case parserResult of
          Nothing -> newProg
          Just (cmd, _) ->
            { newProg | currentState <- (evalCmdList cmd initial_state) }
            |> showDebug

showDebug : Program -> Program
showDebug prog =
    { prog | debug <- (toString prog.currentState)}
