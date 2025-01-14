import chisel3._

class MulPort extends Bundle{
    val a = Input(UInt(16.W))
    val b = Input(UInt(16.W))
    val out = Output(UInt(32.W))
}

class Mul extends Module {
  val io = IO(new MulPort)

  io.out := io.a*io.b

}

object MulTop extends App {
  (new chisel3.stage.ChiselStage).emitVerilog(new Mul())
}