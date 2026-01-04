// src/games/Game301.ts
import { getPoint, type IGameLogic, type Player, useGameStore } from '@/stores/game'

export class Game301 implements IGameLogic {
  private store = useGameStore()
  start (playerNames: string[]): void {
    this.store.players = playerNames.map(name => ({ name, score: 301, darts: [] }))
  }

  throw (input: string): void {
    const points = getPoint(input)
    const currentScore = this.store.players[this.store.currentPlayerIndex].score
    if (currentScore - points >= 0) {
      this.store.players[this.store.currentPlayerIndex].darts.push(input)
      this.store.players[this.store.currentPlayerIndex].score -= points
    } else {
      this.store.players[this.store.currentPlayerIndex].darts.push('0')
    }
  }

  getScore (): number {
    return this.store.players[this.store.currentPlayerIndex].score
  }

  nextPlayer (): number {
    this.store.currentPlayerIndex = (this.store.currentPlayerIndex + 1) % this.store.players.length
    return this.store.currentPlayerIndex
  }

  isWon (): boolean {
    return this.store.players[this.store.currentPlayerIndex].score === 0
  }

  getPlayers (): Player [] {
    return this.store.players ?? []
  }

  gameName (): string {
    return '301'
  }
}
