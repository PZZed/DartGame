import { log } from 'node:console'
import { defineStore } from 'pinia'
import { ref } from 'vue'
// src/stores/game.ts
import { Game301 } from '@/games/301'
import { GameGolf } from '@/games/golf'

export interface Player {
  name: string
  score: number
  darts: string[]
}

export const useGameStore = defineStore('game', () => {
  let game: IGameLogic = new Game301()
  const playersNames = ref<string[]>([])
  const thrownDart = ref(0)
  const players = ref<Player[]>()
  const currentPlayerIndex = ref(0)
  const setGame = (type: string, inputNames: string[]) => {
    game = type === '301' ? new Game301() : new GameGolf()
    playersNames.value = inputNames
  }

  function startGame () {
    game.start(playersNames.value)
    speak(players.value[currentPlayerIndex.value]?.name + 'a toi de jouer !')
  }

  function throwDart (input: string) {
    console.log(input)
    if (thrownDart.value < 3) {
      game.throw(input)
      thrownDart.value++
    } else {
      alert('C\'est au joueur suivant')
    }
  }

  function getScore () {
    return game.getScore()
  }

  function isWon () {
    return game.isWon()
  }

  function getGameName () {
    return game.gameName
  }

  function nextPlayer () {
    thrownDart.value = 0
    const nextPlayer = game.nextPlayer()
    currentPlayerIndex.value = nextPlayer
    speak(players.value[currentPlayerIndex.value]?.name + 'a toi de jouer !')
    return nextPlayer
  }

  return { game, playersNames, setGame, startGame, throwDart, getScore, isWon, players, getGameName, nextPlayer, thrownDart, currentPlayerIndex }
})

// src/stores/game.ts

export interface IGameLogic {
  start: (players: string[]) => void
  throw: (points: string) => void
  getScore: () => number
  nextPlayer: () => number
  isWon: () => boolean
  getPlayers: () => Player[]
  gameName: () => string
}

export function getPoint (input: string): number {
  if (!input && input.length > 0) {
    return 0
  }

  const firstChar = input[0]?.toUpperCase()
  let multiplier = 1
  let numberPart = input

  if (firstChar === 'T') {
    multiplier = 3
    numberPart = input.slice(1)
    if (numberPart === '19') {
      speak('Triple dineuf')
    } else {
      speak('Triple' + numberPart)
    }
  } else if (firstChar === 'D') {
    multiplier = 2
    numberPart = input.slice(1)
    if (numberPart === '19') {
      speak('Double dineuf')
    } else {
      speak('Double' + numberPart)
    }
  } else {
    if (numberPart === '19') {
      speak('dineuf')
    } else if (numberPart === '0') {
      speak('T\'es vraiment nul !')
    } else {
      speak(numberPart)
    }
  }

  const value = Number.parseInt(numberPart)
  if (Number.isNaN(value)) {
    return 0
  }

  return value * multiplier
}

// src/utils/speak.ts
export function speak (text: string) {
  if (!('speechSynthesis' in window)) {
    return
  }

  const utterance = new SpeechSynthesisUtterance(text)
  utterance.lang = 'fr-FR'
  utterance.rate = 1
  utterance.pitch = 1

  window.speechSynthesis.cancel() // coupe la phrase précédente
  window.speechSynthesis.speak(utterance)
}
