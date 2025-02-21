# Routes

# /login POST

Get login jwt token (valid 30 minutes)

## Request
Content-Type: application/json
```json
{
    "username":"your username",
    "password":"your hashed password"
}
```

## Response
### OK
Content-Type: text/plain<br>
token

### BAD_REQUEST
Content-Type: text/plain<br>
"request not valid"

### UNAUTHORIZED
Content-Type: text/plain<br>
"username and password do not match"

# /register POST

Register username and password pair, and get login jwt token (valid 30 minutes)

## Request
Content-Type: application/json
```json
{
    "username":"your username",
    "password":"your hashed password"
}
```

## Response
### OK
Content-Type: text/plain<br>
token

### BAD_REQUEST
Content-Type: text/plain<br>
"request not valid"

### CONFLICT
Content-Type: text/plain<br>
"username already registered"

# /validate POST

Check if the jwt is valid (not expired)

## Request
Authorization: Bearer \<token\>

## Response
### OK
Content-Type: text/plain<br>
"jwt valid"

### BAD_REQUEST
Content-Type: text/plain<br>
"request not valid"

### FORBIDDEN
Content-Type: text/plain<br>
"jwt invalid"

# /refresh POST

Get a new token (valid 30 minutes) if your current token did not expire

## Request
Authorization: Bearer \<token\><br>
Content-Type: application/json
```json
{
    "username":"your username"
}
```

## Response
### OK
Content-Type: text/plain<br>
token

### BAD_REQUEST
Content-Type: text/plain<br>
"request not valid"

### FORBIDDEN
Content-Type: text/plain<br>
"jwt invalid"

# /daily

Get anime ids for the daily anime

## Request
query param
day (optional, default current day): dd/mm/yyyy

## Response
Content-Type: application/json
```json
{
    "screenshotGuess":["screenshotGuessId1", "screenshotGuessId2", "screenshotGuessId3"],
    "characterGuess":[
        ["characterGuess1Id1", "characterGuess1Id2", "characterGuess1Id3", "characterGuess1Id4"],
        ["characterGuess2Id1", "characterGuess2Id2", "characterGuess2Id3", "characterGuess2Id4"],
        ["characterGuess3Id1", "characterGuess3Id2", "characterGuess3Id3", "characterGuess3Id4"]
    ],
    "animeGuess":"animeGuessId"
}
```
